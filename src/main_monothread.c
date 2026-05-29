#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define HASH_INIT_BITS 16   // 2^16 buckets au départ

typedef struct KmerEntry {
    char *kmer;
    long count;
    struct KmerEntry *next;
} KmerEntry;

typedef struct {
    KmerEntry **buckets;
    size_t nbuckets;
    size_t size; // nombre de k-mers distincts
} KmerTable;

/* djb2 hash, borné à la longueur k */
static uint64_t hash_kmer(const char *s, int k) {
    uint64_t h = 5381;
    for (int i = 0; i < k; i++)
        h = ((h << 5) + h) + (unsigned char)s[i];
    return h;
}

static void init_kmer_table(KmerTable *t) {
    t->nbuckets = (1u << HASH_INIT_BITS);
    t->size = 0;
    t->buckets = calloc(t->nbuckets, sizeof(KmerEntry *));
    if (!t->buckets) { perror("calloc"); exit(1); }
}

static void rehash(KmerTable *t) {
    size_t new_n = t->nbuckets * 2;
    KmerEntry **nb = calloc(new_n, sizeof(KmerEntry *));
    if (!nb) { perror("calloc"); exit(1); }
    for (size_t i = 0; i < t->nbuckets; i++) {
        KmerEntry *e = t->buckets[i];
        while (e) {
            KmerEntry *next = e->next;
            size_t idx = hash_kmer(e->kmer, (int)strlen(e->kmer)) & (new_n - 1);
            e->next = nb[idx];
            nb[idx] = e;
            e = next;
        }
    }
    free(t->buckets);
    t->buckets = nb;
    t->nbuckets = new_n;
}

/* ajoute un k-mer (pointeur sur le buffer + longueur, pas de \0 requis) */
static void add_kmer(KmerTable *t, const char *kmer, int k) {
    uint64_t h = hash_kmer(kmer, k);
    size_t idx = h & (t->nbuckets - 1);
    for (KmerEntry *e = t->buckets[idx]; e; e = e->next) {
        if (memcmp(e->kmer, kmer, k) == 0 && e->kmer[k] == '\0') {
            e->count++;
            return;
        }
    }
    /* nouveau k-mer */
    KmerEntry *e = malloc(sizeof(KmerEntry));
    e->kmer = malloc(k + 1);
    memcpy(e->kmer, kmer, k);
    e->kmer[k] = '\0';
    e->count = 1;
    e->next = t->buckets[idx];
    t->buckets[idx] = e;
    t->size++;
    if (t->size > t->nbuckets * 3 / 4) rehash(t);
}

static void free_table(KmerTable *t) {
    for (size_t i = 0; i < t->nbuckets; i++) {
        KmerEntry *e = t->buckets[i];
        while (e) { KmerEntry *n = e->next; free(e->kmer); free(e); e = n; }
    }
    free(t->buckets);
}

/* lit tout le fichier en mémoire */
static char *read_file(const char *filename, long *size_out) {
    FILE *f = fopen(filename, "rb");
    if (!f) { perror("Error opening file"); exit(1); }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(size + 1);
    if (!buf) { perror("malloc"); exit(1); }
    if (fread(buf, 1, size, f) != (size_t)size) {
        fprintf(stderr, "Error reading file\n"); exit(1);
    }
    buf[size] = '\0';
    fclose(f);
    *size_out = size;
    return buf;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input_file> <k>\n", argv[0]);
        return EXIT_FAILURE;
    }
    const char *input_file = argv[1];
    int k = atoi(argv[2]);
    if (k <= 0) {
        fprintf(stderr, "Error: k must be a positive integer.\n");
        return EXIT_FAILURE;
    }

    long file_size;
    char *data = read_file(input_file, &file_size);

    KmerTable table;
    init_kmer_table(&table);

    for (long i = 0; i <= file_size - k; i++)
        add_kmer(&table, data + i, k);

    printf("Results:\n");
    for (size_t b = 0; b < table.nbuckets; b++)
        for (KmerEntry *e = table.buckets[b]; e; e = e->next)
            printf("%s: %ld\n", e->kmer, e->count);

    free_table(&table);
    free(data);
    return 0;
}