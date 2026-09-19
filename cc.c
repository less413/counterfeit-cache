#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_CORES 1

int main(int ac, char** av) {
    if (ac < 3 || ac > 6) {
        printf("\nUsage: %s <protocol> <input_file> [<cache_size> [<associativity> [<block_size>]]]\n\n", av[0]);
        return -1;
    }

    char* protocol = av[1];
    char* input_file = av[2];
    ulong cache_size = 4096;
    ulong associativity = 2;
    ulong block_size = 32;

    if (ac >= 4) {
        cache_size = atoi(av[3]);
    }

    if (ac >= 5) {
        associativity = atoi(av[4]);
    }


    if (ac >= 6) {
        block_size = atoi(av[5]);
    }

    printf("\n");
    printf("protocol: %s\n", protocol);
    printf("input_file: %s\n", input_file);
    printf("cache_size: %ld\n", cache_size);
    printf("associativity: %ld\n", associativity);
    printf("block_size: %ld\n", block_size); // assume: block_size is a power of 2
    printf("\n");

    ulong col_size = cache_size / associativity; // assume: col_size is a NON-ZERO power of 2
    ulong num_sets = col_size / block_size; // assume: num_sets is NON-ZERO

    uint idx_bits = 63 - __builtin_clzl(num_sets);
    uint off_bits = 63 - __builtin_clzl(block_size);
    uint idx_off_bits = 63 - __builtin_clzl(col_size); // = idx_bits + off_bits

    printf("idx_bits: %d\n", idx_bits);
    printf("off_bits: %d\n", off_bits);
    printf("idx_off_bits: %d\n", idx_off_bits);
    printf("\n");

    char* filename = malloc(strlen(input_file) + 8); // <bench_name>_<i>.data'\0'

    // open file/s
    FILE* fds[NUM_CORES];
    for (int i = 0; i < NUM_CORES; ++i) {
        sprintf(filename, "%s_%d.data", input_file, i);
        fds[i] = fopen(filename, "r"); // TODO read actual input file
        if (fds[i] == NULL) {
            printf("ERROR: file %s is missing", filename);
            return -1;
        }
    }

    // read file
    // TODO extend to four cores (how?)
    uint inst, addr;
    while (fscanf(fds[0], "%u %x", &inst, &addr) == 2) {
        // printf("the line is type %u, address %x\n", inst, addr);
    }
}

