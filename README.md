# --- Préparation ---
```
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j
cd ..
```

# Vérifie la taille du fichier
```
ls -lh pi.txt
```

# --- Vérification de correction (les deux doivent donner le même résultat) ---
```
./build/k-mer pi.txt 3 | sort > out_mono.txt
./build/k-mer-mt pi.txt 3 | sort > out_mt.txt
diff out_mono.txt out_mt.txt && echo "OK: resultats identiques"
```

# --- Mesures mono-threade ameliore (plusieurs valeurs de k) ---
```
for k in 1 3 5 7 10; do
    echo "=== k=$k mono ==="
    for run in 1 2 3; do
        { time ./build/k-mer pi.txt $k > /dev/null ; } 2>> time_mono_k${k}.txt
    done
    echo "--- temps k=$k ---"
    cat time_mono_k${k}.txt
done
```

# --- Mesures multithreade : scalabilite selon le nombre de threads ---
```
for k in 3 7; do
    for t in 1 2 4 8; do
        echo "=== k=$k threads=$t ==="
        export OMP_NUM_THREADS=$t
        for run in 1 2 3; do
            { time ./build/k-mer-mt pi.txt $k > /dev/null ; } 2>> time_mt_k${k}_t${t}.txt
        done
        echo "--- temps k=$k t=$t ---"
        cat time_mt_k${k}_t${t}.txt
    done
done
unset OMP_NUM_THREADS
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