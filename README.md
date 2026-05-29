# --- Préparation ---
```
rm -rf build
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j
cd ..
```

# Vérifie la taille du fichier
```
ls -lh ./inputs/pi.txt # 100 000 caractères
```

# --- Vérification de correction (les deux doivent donner le même résultat) ---
```
./build/k-mer ./inputs/pi.txt 3 | sort > out_mono.txt
./build/k-mer-mt ./inputs/pi.txt 3 | sort > out_mt.txt
diff out_mono.txt out_mt.txt && echo "OK: resultats identiques"
```

# --- Mesures mono-threade ameliore (plusieurs valeurs de k) ---
```
for k in 1 3 5 7 10; do
    echo "=== k=$k mono ==="
    for run in 1 2 3; do
        { time ./build/k-mer ./inputs/pi.txt $k > /dev/null ; } 2>> time_mono_k${k}.txt
    done
    cat time_mono_k${k}.txt
done
```

Résultats
```
=== k=1 mono ===
./build/k-mer ./inputs/pi.txt $k > /dev/null  0,14s user 0,02s system 88% cpu 0,173 total
./build/k-mer ./inputs/pi.txt $k > /dev/null  0,13s user 0,01s system 97% cpu 0,147 total
./build/k-mer ./inputs/pi.txt $k > /dev/null  0,13s user 0,01s system 98% cpu 0,143 total
=== k=3 mono ===
./build/k-mer ./inputs/pi.txt $k > /dev/null  0,26s user 0,01s system 99% cpu 0,270 total
./build/k-mer ./inputs/pi.txt $k > /dev/null  0,26s user 0,01s system 99% cpu 0,272 total
./build/k-mer ./inputs/pi.txt $k > /dev/null  0,26s user 0,01s system 99% cpu 0,273 total
=== k=5 mono ===
./build/k-mer ./inputs/pi.txt $k > /dev/null  0,42s user 0,01s system 99% cpu 0,428 total
./build/k-mer ./inputs/pi.txt $k > /dev/null  0,41s user 0,01s system 99% cpu 0,424 total
./build/k-mer ./inputs/pi.txt $k > /dev/null  0,43s user 0,01s system 94% cpu 0,469 total
=== k=7 mono ===
./build/k-mer ./inputs/pi.txt $k > /dev/null  8,76s user 0,14s system 99% cpu 8,943 total
./build/k-mer ./inputs/pi.txt $k > /dev/null  8,73s user 0,13s system 99% cpu 8,885 total
./build/k-mer ./inputs/pi.txt $k > /dev/null  8,89s user 0,13s system 99% cpu 9,023 total
=== k=10 mono ===
./build/k-mer ./inputs/pi.txt $k > /dev/null  29,02s user 3,50s system 97% cpu 33,455 total
./build/k-mer ./inputs/pi.txt $k > /dev/null  28,08s user 1,34s system 98% cpu 29,844 total
./build/k-mer ./inputs/pi.txt $k > /dev/null  28,64s user 1,31s system 98% cpu 30,260 total
```

# --- Mesures multithreade : scalabilite selon le nombre de threads ---
```
for k in 3 7; do
    for t in 1 2 4 8; do
        echo "=== k=$k threads=$t ==="
        export OMP_NUM_THREADS=$t
        for run in 1 2 3; do
            { time ./build/k-mer-mt ./inputs/pi.txt $k > /dev/null ; } 2>> time_mt_k${k}_t${t}.txt
        done
        cat time_mt_k${k}_t${t}.txt
    done
done
unset OMP_NUM_THREADS
```

Résultats

```
=== k=3 threads=1 ===
./build/k-mer-mt ./inputs/pi.txt $k > /dev/null  0,29s user 0,01s system 93% cpu 0,322 total
./build/k-mer-mt ./inputs/pi.txt $k > /dev/null  0,29s user 0,01s system 99% cpu 0,303 total
./build/k-mer-mt ./inputs/pi.txt $k > /dev/null  0,29s user 0,01s system 99% cpu 0,302 total
=== k=3 threads=2 ===
./build/k-mer-mt ./inputs/pi.txt $k > /dev/null  0,30s user 0,01s system 190% cpu 0,164 total
./build/k-mer-mt ./inputs/pi.txt $k > /dev/null  0,29s user 0,01s system 191% cpu 0,158 total
./build/k-mer-mt ./inputs/pi.txt $k > /dev/null  0,30s user 0,01s system 191% cpu 0,159 total
=== k=3 threads=4 ===
./build/k-mer-mt ./inputs/pi.txt $k > /dev/null  0,30s user 0,01s system 361% cpu 0,086 total
./build/k-mer-mt ./inputs/pi.txt $k > /dev/null  0,30s user 0,01s system 361% cpu 0,086 total
./build/k-mer-mt ./inputs/pi.txt $k > /dev/null  0,31s user 0,01s system 359% cpu 0,087 total
=== k=3 threads=8 ===
./build/k-mer-mt ./inputs/pi.txt $k > /dev/null  0,47s user 0,01s system 539% cpu 0,089 total
./build/k-mer-mt ./inputs/pi.txt $k > /dev/null  0,44s user 0,01s system 586% cpu 0,076 total
./build/k-mer-mt ./inputs/pi.txt $k > /dev/null  0,47s user 0,01s system 531% cpu 0,090 total
=== k=7 threads=1 ===
./build/k-mer-mt ./inputs/pi.txt $k > /dev/null  8,66s user 0,21s system 99% cpu 8,882 total
./build/k-mer-mt ./inputs/pi.txt $k > /dev/null  8,70s user 0,19s system 99% cpu 8,915 total
./build/k-mer-mt ./inputs/pi.txt $k > /dev/null  8,75s user 0,20s system 99% cpu 8,979 total
=== k=7 threads=2 ===
./build/k-mer-mt ./inputs/pi.txt $k > /dev/null  13,83s user 0,34s system 176% cpu 8,048 total
./build/k-mer-mt ./inputs/pi.txt $k > /dev/null  13,38s user 0,30s system 179% cpu 7,639 total
./build/k-mer-mt ./inputs/pi.txt $k > /dev/null  13,49s user 0,34s system 177% cpu 7,794 total
=== k=7 threads=4 ===
./build/k-mer-mt ./inputs/pi.txt $k > /dev/null  23,96s user 0,84s system 307% cpu 8,062 total
./build/k-mer-mt ./inputs/pi.txt $k > /dev/null  23,95s user 0,58s system 303% cpu 8,092 total
./build/k-mer-mt ./inputs/pi.txt $k > /dev/null  23,53s user 0,62s system 307% cpu 7,849 total
=== k=7 threads=8 ===
./build/k-mer-mt ./inputs/pi.txt $k > /dev/null  46,97s user 0,90s system 502% cpu 9,529 total
./build/k-mer-mt ./inputs/pi.txt $k > /dev/null  45,26s user 1,16s system 514% cpu 9,018 total
./build/k-mer-mt ./inputs/pi.txt $k > /dev/null  45,43s user 1,01s system 519% cpu 8,943 total
```

# --- Système ---
```
sysctl -n hw.ncpu # Retourne 8
sysctl -n hw.physicalcpu   # Retourne 8
system_profiler SPHardwareDataType | grep -E "Chip|Cores|Memory"
# 2026-05-29 09:47:13.207 system_profiler[56677:1989864] hw.cpufamily: 0xfa33415e
# Chip: Apple M3
# Total Number of Cores: 8 (4 Performance and 4 Efficiency)
# Memory: 16 GB
```