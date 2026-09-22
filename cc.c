#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

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

    uint col_size = cache_size / associativity; // assume: col_size is a NON-ZERO power of 2
    uint num_sets = col_size / block_size; // assume: num_sets is NON-ZERO

    uint idx_bits = 31 - __builtin_clz(num_sets);
    uint off_bits = 31 - __builtin_clz(block_size);
    uint idx_off_bits = 31 - __builtin_clz(col_size); // = idx_bits + off_bits
    uint idx_ones = idx_bits ? 0xFFFFFFFF >> (32 - idx_bits) : 0; // ternary op. to handle idx_bits == 0
                                                                  // because >> by 32 is undefined

    fprintf(stderr, "idx_bits: %d\n", idx_bits);
    fprintf(stderr, "off_bits: %d\n", off_bits);
    fprintf(stderr, "idx_off_bits: %d\n", idx_off_bits);
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
    uint cache_tag[num_sets][associativity];
    uint cache_state[num_sets][associativity];
    uint t_used[num_sets][associativity];
    uint t_next = 1;

    for (uint i = 0; i < num_sets; ++i) {
        for (uint j = 0; j < associativity; ++j) {
            cache_state[i][j] = 0;
            t_used[i][j] = 0;
        }
    }

    // initialize statistics
    ulong total_cycles = 0;
    ulong comp_cycles = 0;
    ulong mem_instrs = 0;
    ulong cache_p_hits = 0;
    ulong cache_misses = 0;

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

        uint has_match = 0;
        uint tag = addr >> idx_off_bits;
        uint idx = (addr >> off_bits) & idx_ones;

        fprintf(stderr, "address: %x\n", addr);
        fprintf(stderr, "tag: %x\n", tag);
        fprintf(stderr, "idx: %x\n", idx);

        uint way = 6767; // way to be updated
        uint lru_t = UINT_MAX;
        
        // 1. check set for match
        for (uint j = 0; j < associativity; ++j) {
            fprintf(stderr, "%d,%d: tag: %x  state: %d\n", idx, j, cache_tag[idx][j], cache_state[idx][j]);
            if (cache_tag[idx][j] == tag && cache_state[idx][j]) {
                // cache hit
                fprintf(stderr, "wow cache hit\n");
                has_match = 1;
                way = j;
                cache_p_hits += 1;
                break;
            }

            // track lru way
            if (t_used[idx][j] < lru_t) {
                lru_t = t_used[idx][j];
                way = j;
            }
        }

        // 2. (if no match) evict/load block
        if (!has_match) {
            fprintf(stderr, "oh no cache miss\n");
            ++cache_misses;

            // if block is dirty
            if (cache_state[idx][way] == 3) {
                // FLUSH to mem
                total_cycles += 100;
                data_traffic += block_size;
            }

            // LOAD new block
            cache_tag[idx][way] = tag;
            if (inst == 0) { // TODO separate load/store into different functions
                cache_state[idx][way] = 2;
            }
            total_cycles += 100;
            data_traffic += block_size;
        }
        
        fprintf(stderr, "way: %d\n\n", way);

        // 3. for store instructions: update state to dirty
        if (inst == 1) { // TODO separate load/store into different functions
            cache_state[idx][way] = 3;
        }

        // 4. update t
        t_used[idx][way] = t_next;
        ++t_next;
    }

    printf("%ld\n", total_cycles);
    printf("%ld\n", comp_cycles);
    printf("%ld\n", mem_instrs);
    printf("%ld\n", total_cycles - comp_cycles); // idle cycles
    printf("%ld:%ld\n", cache_p_hits, cache_misses);
    printf("%ld\n", data_traffic);

}

