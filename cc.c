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

// custom parser that assumes each line contains "<0-2> 0x<32b hex>"
static inline int read_line(const char** pp, const char* end, uint* inst, uint* addr) {
    const char* file_pos = *pp;
    if (file_pos >= end) {
        return 0; // stop file parsing
    }

    *inst = (uint)(*file_pos++ - '0');

    file_pos += 3; // skip the " 0x"

    uint32_t x = 0;
    // this parser assumes the hex number only contains 0-9 and a-f
    while (file_pos < end) {
        unsigned char c = (unsigned char)*file_pos;
        if (c >= '0' && c <= '9') {
            x = (x << 4) + (c - '0');
        } else {
            c += 9;
            if (c & 16) { // if c is '\n'
                break;
            }
            // c is A-F or a-f
            x = (x << 4) + (c & 15);
        }
        ++file_pos;
    }

    *addr = x;

    if (file_pos < end && *file_pos == '\n') {
        ++file_pos;
    }
    *pp = file_pos;

    return 1;
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
    ulong mem_instrs = 0;

    // ** SINGLE CORE **
    // initialize cache
    // TODO extend to 4 cores
    Cache* cache = cache_create(cache_size, associativity, block_size);

    // initialize bus
    Bus* bus = bus_create();

    // read file
    uint inst, addr;
    // TODO extend to 4 cores
    while (read_line(&file_pos[0], file_end[0], &inst, &addr)) { // call custom line parser
        if (inst == 2) {
            // computation instr
            comp(cache, addr);
            continue;
        }

        // either load (0) or store (1)
        ++mem_instrs;
        if (inst == 0) {
            cache_pr_rd(cache, bus, addr);
        } else {
            cache_pr_wr(cache, bus, addr);
        }
    }

    // print results
    // 1. total execution cycles per core "aaa bbb ccc ddd"
    printf("%lu\n", cache->total_cycles);
    // 2. compute cycles per core "aaa bbb ccc ddd"
    printf("%lu\n", cache->comp_cycles);
    // 3. load/store instrs per core "aaa bbb ccc ddd"
    printf("%lu\n", mem_instrs);
    // 4. idle cycles per core "aaa bbb ccc ddd"
    printf("%lu\n", cache->total_cycles - cache->comp_cycles); // idle cycles
    // 5. cache hit:miss per core "h:m h:m h:m h:m"
    printf("%lu:%lu\n", cache->p_hits, cache->misses);
    // 6. bus data traffic "bytes"
    printf("%lu\n", bus->data_traffic);
    // 7. bus invalidations/updates
    // 8. cache private:shared hits per core "p:s p:s p:s p:s"

    // free resources
    for (int i = 0; i < NUM_CORES; ++i) {
        close(fds[i]);
    }
    bus_free(bus);
    cache_free(cache);
}

