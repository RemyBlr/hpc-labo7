# Laboratoire 7 — Parallélisme des tâches

## Première partie : Analyse des k-mers

### Environnement de test

| Élément | Valeur |
|---|---|
| Machine | Apple M3 |
| Cœurs | 8 (4 Performance + 4 Efficiency) |
| Mémoire | 16 Go |
| Fichier d'entrée | 50 millions de chiffres décimaux (~50 Mo) |
| Compilation | `-O3 -mcpu=native`, OpenMP via libomp |
| Méthode | 3 exécutions par configuration, on retient le temps médian (`real`) |

---

### 1. Introduction

Le programme analyse un fichier texte et compte la fréquence d'apparition de chaque sous-séquence de longueur `k` (k-mer). Pour une entrée de `N` caractères il existe `N - k + 1` positions de départ, donc autant de k-mers à comptabiliser. La sortie est la liste de tous les k-mers distincts accompagnés de leur nombre d'occurrences.

Deux exécutables sont produits : une version mono-threadée améliorée (`k-mer`) et une version multithreadée OpenMP (`k-mer-mt`).

---

### 2. Inefficacités du code fourni et améliorations apportées

Le code de départ présentait deux défauts rédhibitoires.

**Réouverture du fichier à chaque k-mer.** La fonction `read_kmer` exécutait `fopen` / `fseek` / `fgetc` × k / `fclose` pour **chaque** position du fichier. Sur 50 millions de positions, cela représente 50 millions d'ouvertures de fichier — un coût d'I/O et d'appels système totalement disproportionné. La correction consiste à lire le fichier **une seule fois** entièrement en mémoire (`read_file`), puis à travailler directement sur le buffer : un k-mer à la position `i` est simplement le pointeur `data + i`, sans aucune copie ni relecture.

**Recherche linéaire dans la table.** La fonction `add_kmer` parcourait tout le tableau existant (`strcmp` sur chaque entrée) avant d'insérer ou d'incrémenter. La complexité globale était donc `O(N × D)` où `D` est le nombre de k-mers distincts. Pour de grands `k`, `D` devient énorme (jusqu'à ~10 millions d'entrées pour k=7), rendant l'algorithme quadratique et inutilisable. La correction remplace le tableau par une **table de hachage** (hash djb2, buckets en chaînage, redimensionnement quand le facteur de charge dépasse 0,75). L'insertion/incrémentation passe ainsi en `O(1)` amorti, ramenant la complexité globale à `O(N)`.

Le comportement et l'interface en ligne de commande (`<input_file> <k>`) restent strictement identiques à l'original.

---

### 3. Performances de la version mono-threadée

Temps médian (`real`) sur 50 Mo :

| k | Temps mono (s) | k-mers distincts (ordre de grandeur) |
|---|---|---|
| 1 | 0,147 | 10 |
| 3 | 0,272 | 1 000 |
| 5 | 0,428 | 100 000 |
| 7 | 8,89 | ~10 000 000 |
| 10 | 30,3 | ~50 000 000 (≈ toutes positions distinctes) |

L'analyse de cette baseline est révélatrice. Tant que le nombre de k-mers distincts reste petit (k ≤ 5), le temps croît doucement : la table de hachage tient entièrement dans les caches CPU et chaque accès est quasi gratuit. Le travail est dominé par le simple parcours linéaire des 50 M de positions.

À partir de **k=7**, le temps fait un bond de 0,43 s à 8,9 s (×20). Le nombre de k-mers distincts (~10 M) dépasse alors largement la capacité des caches L1/L2 : chaque insertion provoque un défaut de cache vers la RAM. Le programme devient **memory-bound** plutôt que compute-bound. À k=10, presque chaque position produit un k-mer unique : la table contient des dizaines de millions d'entrées, les défauts de cache et la pression sur l'allocateur (`malloc` par entrée) dominent tout, d'où les 30 s.

Ce changement de régime — d'un coût dominé par le parcours à un coût dominé par les accès mémoire aléatoires dans la table — est le fait marquant de la baseline, et il conditionne entièrement le comportement de la version parallèle.

---

### 4. Stratégie de parallélisation

La parallélisation utilise OpenMP avec l'approche **« table locale par thread + fusion »** :

1. **Répartition du travail.** Les `N - k + 1` positions de départ sont découpées en plages contiguës de taille égale, une par thread (`chunk = ceil(total_positions / nthreads)`). Chaque thread traite l'intervalle `[start, end)` de positions.

2. **Comptage sans contention.** Chaque thread possède sa **propre table de hachage locale**. Pendant toute la phase de comptage, il n'y a donc **aucun verrou ni accès partagé** : les threads travaillent en totale indépendance, ce qui maximise le parallélisme.

3. **Gestion des cas limites / chevauchement.** Le buffer complet est partagé en lecture seule et se termine par un sentinel `\0`. Un thread qui traite la position `i` lit `data[i .. i+k-1]`, ce qui peut empiéter de `k-1` octets sur la plage du thread suivant — mais comme la lecture porte sur le buffer global, **aucun k-mer n'est perdu ni dupliqué** à la frontière. Chaque position de départ valide est traitée exactement une fois, par un seul thread. C'est un overlap de **lecture** (sûr), pas de comptage.

4. **Fusion.** À la fin, chaque table locale est fusionnée dans une table globale au sein d'une section `#pragma omp critical`. Le coût de la fusion est proportionnel au nombre de k-mers **distincts** par thread, pas au nombre total de positions ; il est donc négligeable quand `D` est petit, mais devient significatif quand `D` est grand (voir analyse k=7).

---

### 5. Comparaison mono-threadée vs multithreadée

#### Cas k=3 — comportement idéal

| Threads | Temps (s) | Speedup vs 1 thread | CPU% observé |
|---|---|---|---|
| 1 | 0,302 | 1,00× | ~99 % |
| 2 | 0,159 | 1,90× | ~191 % |
| 4 | 0,086 | 3,51× | ~360 % |
| 8 | 0,081 | 3,73× | ~550 % |

Avec seulement 1 000 k-mers distincts, chaque table locale tient intégralement dans le cache : le travail est purement compute-bound et parfaitement divisible. Le speedup est quasi linéaire jusqu'à 4 threads (3,5× pour 4 cœurs). Le passage de 4 à 8 threads n'apporte presque rien (3,5× → 3,7×) : les 4 threads supplémentaires s'exécutent sur les **cœurs Efficiency**, nettement plus lents que les cœurs Performance. C'est la signature classique d'une architecture hétérogène big.LITTLE — au-delà de 4 cœurs P, le gain marginal s'effondre.

#### Cas k=7 — saturation mémoire (le cas instructif)

| Threads | Temps real (s) | Temps user (s) | Speedup | CPU% |
|---|---|---|---|---|
| 1 | 8,92 | 8,70 | 1,00× | ~99 % |
| 2 | 7,79 | 13,57 | 1,15× | ~177 % |
| 4 | 8,06 | 23,81 | 1,11× | ~306 % |
| 8 | 9,02 | 45,89 | 0,99× | ~512 % |

Ici, **ajouter des threads n'accélère pas le programme** : le temps real reste autour de 8–9 s quel que soit le nombre de threads. Pire, le temps **user explose** (8,7 s → 46 s, soit ×5,3 pour 8 threads) alors que le résultat calculé est identique. Le CPU% augmente bien (les cœurs tournent), mais ce travail supplémentaire est purement gaspillé.

L'explication tient à deux goulots d'étranglement :

- **Bande passante mémoire saturée.** Avec ~10 M de k-mers distincts, chaque table locale fait plusieurs centaines de Mo et déborde très largement des caches. Chaque insertion est un accès aléatoire à la RAM. Plusieurs threads se partagent la **même bande passante mémoire** : ils ne se parallélisent pas, ils se mettent en file d'attente sur le contrôleur mémoire. Le temps user gonfle car chaque cœur passe son temps à attendre des lignes de cache (temps compté comme « actif »).

- **Coût de la fusion en section critique.** À 8 threads, ce sont 8 tables de ~10 M d'entrées à fusionner séquentiellement dans la table globale. Cette phase, sérialisée par `omp critical`, ne profite pas du parallélisme et son poids croît avec le nombre de threads.

**Conclusion de la comparaison.** La parallélisation par découpage est très efficace **quand le problème est compute-bound et que l'empreinte mémoire par thread tient en cache** (petit `k`) : on atteint alors un speedup proche du nombre de cœurs Performance. Dès que le problème devient memory-bound (grand `k`, grand nombre de k-mers distincts), ajouter des threads est contre-productif : la bande passante mémoire devient le facteur limitant, et la duplication des tables locales aggrave encore la pression mémoire. Sur cette architecture M3, le sweet spot pratique est de **4 threads** (cœurs Performance), au-delà duquel les cœurs Efficiency et la saturation mémoire annulent tout gain.

Des pistes d'amélioration possibles seraient : une table de hachage partagée par verrous fins (sharding) pour éviter la fusion finale coûteuse, ou une fusion arborescente parallèle (réduction) plutôt que séquentielle, afin d'atténuer le goulot du `critical` dans le cas grand `k`.