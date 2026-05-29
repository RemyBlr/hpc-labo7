#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_KMER 100

typedef struct {
    int count;
    char kmer[MAX_KMER];
} KmerEntry;

typedef struct {
    KmerEntry *entries;
    int count;
    int capacity;
} KmerTable;

void init_kmer_table(KmerTable *table) {
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void read_kmer(const char *filename, long position, int k, char *kmer) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        perror("Error opening file");
        exit(1);
    }
    fseek(f, position, SEEK_SET);
    for (int i = 0; i < k; i++) {
        int c = fgetc(f);
        if (c == EOF) {
            fprintf(stderr, "Error: Reached end of file before reading k-mer.\n");
            fclose(f);
            exit(1);
        }
        kmer[i] = (char)c;
    }
    kmer[k] = '\0';
    fclose(f);
}

void add_kmer(KmerTable *table, const char *kmer) {
    for (int i = 0; i < table->count; i++) {
        if (strcmp(table->entries[i].kmer, kmer) == 0) {
            table->entries[i].count++;
            return;
        }
    }

    if (table->count >= table->capacity) {
        table->capacity = (table->capacity == 0) ? 1 : table->capacity * 2;
        table->entries = realloc(table->entries, table->capacity * sizeof(KmerEntry));
        if (!table->entries) {
            perror("Error reallocating memory for k-mer table");
            exit(1);
        }
    }

    strcpy(table->entries[table->count].kmer, kmer);
    table->entries[table->count].count = 1;
    table->count++;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input_file> <k>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *input_file = argv[1];
    int k = atoi(argv[2]);
    char kmer[MAX_KMER];


    if (k <= 0) {
        fprintf(stderr, "Error: k must be a positive integer.\n");
        return EXIT_FAILURE;
    }

    FILE *file = fopen(input_file, "r");
    if (!file) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fclose(file);

    KmerTable table;
    init_kmer_table(&table);

    for (long i = 0; i <= file_size - k; i++) {
        read_kmer(input_file, i, k, kmer);
        add_kmer(&table, kmer);
    }

    printf("Results:\n");
    for (int i = 0; i < table.count; i++) {
        printf("%s: %d\n", table.entries[i].kmer, table.entries[i].count);
    }

    free(table.entries);

    return 0;
}