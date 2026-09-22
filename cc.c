#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "cache.h"

#define NUM_CORES 1
#define WORD_SIZE 4

int main(int ac, char** av) {
    if (ac < 3 || ac > 6) {
        fprintf(stderr, "\nUsage: %s <protocol> <input_file> [<cache_size> [<associativity> [<block_size>]]]\n\n", av[0]);
        return -1;
    }

    char* protocol = av[1];
    char* input_file = av[2];
    uint cache_size = 4096;
    uint associativity = 2;
    uint block_size = 32;

    if (ac >= 4) {
        cache_size = atoi(av[3]);
    }

    if (ac >= 5) {
        associativity = atoi(av[4]);
    }


    if (ac >= 6) {
        block_size = atoi(av[5]);
    }

    // validate inputs
    if (block_size < WORD_SIZE) {
        fprintf(stderr, "\nERROR: block_size (%uB) cannot be smaller than a word (%uB)\n\n", block_size, WORD_SIZE);
        return -1;
    }

    fprintf(stderr, "\n");
    fprintf(stderr, "protocol: %s\n", protocol);
    fprintf(stderr, "input_file: %s\n", input_file);
    fprintf(stderr, "cache_size: %d\n", cache_size);
    fprintf(stderr, "associativity: %d\n", associativity);
    fprintf(stderr, "block_size: %d\n", block_size); // assume: block_size is a power of 2
    fprintf(stderr, "\n");

    char* filename = malloc(strlen(input_file) + 8); // <bench_name>_<i>.data'\0'

    // open file/s
    FILE* fds[NUM_CORES];
    for (int i = 0; i < NUM_CORES; ++i) {
        sprintf(filename, "%s_%d.data", input_file, i);
        fds[i] = fopen(filename, "r"); // TODO read actual input file
        if (fds[i] == NULL) {
            fprintf(stderr, "ERROR: file %s is missing", filename);
            return -1;
        }
    }

    // initialize cache
    // TODO extend to 4 cores
    Cache* cache = cache_create(cache_size, associativity, block_size);

    // initialize statistics
    ulong total_cycles = 0;
    ulong comp_cycles = 0;
    ulong mem_instrs = 0;

    ulong data_traffic = 0;

    // read file
    uint inst, addr;
    while (fscanf(fds[0], "%u %x", &inst, &addr) == 2) {
        if (inst == 2) {
            // computation instr
            total_cycles += addr;
            comp_cycles += addr;
            continue;
        }

        // either load (0) or store (1)
        ++mem_instrs;
        if (inst == 0) {
            total_cycles += cache_pr_rd(cache, addr);
        } else {
            total_cycles += cache_pr_wr(cache, addr);
        }
    }

    // print results
    // 1. total execution cycles per core "aaa bbb ccc ddd"
    printf("%ld\n", total_cycles);
    // 2. compute cycles per core "aaa bbb ccc ddd"
    printf("%ld\n", comp_cycles);
    // 3. load/store instrs per core "aaa bbb ccc ddd"
    printf("%ld\n", mem_instrs);
    // 4. idle cycles per core "aaa bbb ccc ddd"
    printf("%ld\n", total_cycles - comp_cycles); // idle cycles
    // 5. cache hit:miss per core "h:m h:m h:m h:m"
    printf("%ld:%ld\n", cache->p_hits, cache->misses);
    // 6. bus data traffic "bytes"
    printf("%ld\n", data_traffic);
    // 7. bus invalidations/updates
    // 8. cache private:shared hits per core "p:s p:s p:s p:s"
}

