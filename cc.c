#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "cc.h"
#include "cache.h"
#include "bus.h"
#include "parse.h"


typedef struct {
    int valid;
    uint inst; // aka label
    uint val;
} Pending;

static void print_row(const ulong* v) {
    for (int i = 0; i < NUM_CORES; ++i)
        printf("%lu%c", v[i], i == NUM_CORES - 1 ? '\n' : ' ');
}

static void print_pair_row(const ulong* a, const ulong* b) {
    for (int i = 0; i < NUM_CORES; ++i)
        printf("%lu:%lu%c", a[i], b[i], i == NUM_CORES - 1 ? '\n' : ' ');
}

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
    int fds[NUM_CORES];
    const char* file_pos[NUM_CORES];
    const char* file_end[NUM_CORES]; // first address outside the file
    size_t file_size[NUM_CORES];

    for (int i = 0; i < NUM_CORES; ++i) {
        sprintf(filename, "%s_%d.data", input_file, i);
        fds[i] = open(filename, O_RDONLY);
        if (fds[i] == -1) {
            fprintf(stderr, "ERROR opening file %s\n", filename);
            return -1;
        }
        struct stat st;
        if (fstat(fds[i], &st) != 0) {
            fprintf(stderr, "ERROR getting file size %s\n", filename);
            return -1;
        }
        file_size[i] = (size_t)st.st_size;
        if (file_size[i] == 0) {
            file_pos[i] = file_end[i] = NULL;
            continue;
        }
        file_pos[i] = mmap(NULL, file_size[i], PROT_READ, MAP_PRIVATE, fds[i], 0);
        if (file_pos[i] == MAP_FAILED) {
            fprintf(stderr, "ERROR loading file %s\n", filename);
            return -1;
        }
        file_end[i] = file_pos[i] + file_size[i];
    }

    free(filename);

    // initialize statistics
    ulong mem_instrs[NUM_CORES] = {0, 0, 0, 0};

    // initialize cache
    Cache* caches[NUM_CORES];
    for (int i = 0; i < NUM_CORES; ++i) {
        caches[i] = cache_create(cache_size, associativity, block_size);
    }

    // initialize bus
    Bus* bus = bus_create(caches);
    
    Pending next[NUM_CORES];
    for (int i = 0; i < NUM_CORES; i++) {
        next[i].valid = read_line(&file_pos[i], file_end[i], &next[i].inst, &next[i].val);
    }

    // notes: issue_time just use cache[idx]->total_cycles;
    // no need to explicitly update issue_time, the functions does it
    while (1) {

        int idx = -1;
        for (int i = 0; i < NUM_CORES; i++) {
            if (!next[i].valid) continue;
            
            if (idx == -1) {
                idx = i;
            } else if (caches[i]->total_cycles < caches[idx]->total_cycles) {
                idx = i;
            }
        }

        if (idx < 0) break;

        Cache* curr_cache = caches[idx];
        uint inst = next[idx].inst;
        uint val = next[idx].val;

        // "other" instr
        if (inst == 2) {
            // TODO computation
            comp(curr_cache, val);
            // continue;
        } else {
            // load (0) or store (1)
            mem_instrs[idx]++;
            if (inst == 0) {
                // TODO handle load logic
                cache_pr_rd(curr_cache, bus, val);
            } else {
                // TODO handle store logic
                cache_pr_wr(curr_cache, bus, val);
            }
        }

        // if end of list, store next as not valid
        next[idx].valid = read_line(&file_pos[idx], file_end[idx], &next[idx].inst, &next[idx].val);
    }

    // while (read_line(&file_pos[0], file_end[0], &inst, &addr)) { // call custom line parser
    //     if (inst == 2) {
    //         // computation instr
    //         comp(caches[0], addr);
    //         continue;
    //     }

    //     // either load (0) or store (1)
    //     ++mem_instrs;
    //     if (inst == 0) {
    //         cache_pr_rd(caches[0], bus, addr);
    //     } else {
    //         cache_pr_wr(caches[0], bus, addr);
    //     }
    // }

    // for printing
    ulong total[NUM_CORES], compute[NUM_CORES], idle[NUM_CORES];
    ulong hits[NUM_CORES], misses[NUM_CORES], priv[NUM_CORES], shared[NUM_CORES];
    for (int i = 0; i < NUM_CORES; ++i) {
        Cache* c = caches[i];
        total[i]   = c->total_cycles;
        compute[i] = c->comp_cycles;
        idle[i]    = c->total_cycles - c->comp_cycles;
        hits[i]    = c->p_hits + c->s_hits;
        misses[i]  = c->misses;
        priv[i]    = c->p_hits;
        shared[i]  = c->s_hits;
    }

    // print results
    print_row(total);                           // 1. total execution cycles per core "aaa bbb ccc ddd"
    print_row(compute);                         // 2. compute cycles per core "aaa bbb ccc ddd"
    print_row(mem_instrs);                      // 3. load/store instrs per core "aaa bbb ccc ddd"
    print_row(idle);                            // 4. idle cycles per core "aaa bbb ccc ddd"
    print_pair_row(hits, misses);               // 5. cache hit:miss per core "h:m h:m h:m h:m"
    printf("%lu\n", bus->data_traffic);         // 6. bus data traffic "bytes"
    printf("%lu\n", bus->invalidations);        // 7. bus invalidations/updates
    print_pair_row(priv, shared);               // 8. cache private:shared hits per core "p:s p:s p:s p:s"

    // free resources
    for (int i = 0; i < NUM_CORES; ++i) {
        close(fds[i]);
    }
    bus_free(bus);
    for (int i = 0; i < NUM_CORES; i++) {
        cache_free(caches[i]);
    }
}

