#include <stdint.h>
#include <stdlib.h>
#include "cc.h"
#include "bus.h"

Bus* bus_create(Cache** caches) {
    Bus* b = malloc(sizeof(Bus));

    // initialize
    b->data_traffic = 0;
    b->invalidations = 0;
    b->t_free = 0;
    for (int i = 0; i < NUM_CORES; ++i) {
        b->caches[i] = caches[i];
    }

    return b;
}

void bus_free(Bus* b) {
    if (!b) return;

    free(b);
}

void bus_flush_mem(Cache* c, Bus* b) {
    c->total_cycles += FLUSH_MEM_CYCLES;
    b->data_traffic += c->block_size;
    b->t_free = c->total_cycles;
}

// #cycles to send one block cache-to-cache; 2N, N = #words
static uint64_t c2c_cycles(Cache* c) {
    return C2C_CYCLES_PER_WORD * (c->block_size / WORD_SIZE);
}

// BusRd for a read miss. Returns the requester's new state: S if shared, else E.
uint32_t bus_rd(Cache* c, Bus* b, uint32_t addr) {
    int shared = 0;
    int dirty = 0;
    for (int j = 0; j < NUM_CORES; ++j) {
        Cache* o = b->caches[j];
        if (o == c) { // skip the requester
            continue;
        }
        int old = cache_bus_rd(o, addr);
        shared |= (old != ST_I);
        dirty |= (old == ST_M);
    }
    uint64_t cycles;
    if (dirty) {
        // M owner flushes once; requester (2N) and memory (100) receive it together
        uint64_t c2c = c2c_cycles(c);
        cycles = c2c > FLUSH_MEM_CYCLES ? c2c : FLUSH_MEM_CYCLES;
    } else {
        cycles = shared ? c2c_cycles(c) : LOAD_MEM_CYCLES;
    }
    c->total_cycles += cycles;
    b->data_traffic += c->block_size;
    b->t_free = c->total_cycles;
    return shared ? ST_S : ST_E;
}

// BusRdX for a write miss, or a write hit in S. Requester ends in M.
void bus_rdx(Cache* c, Bus* b, uint32_t addr) {
    int shared = 0;
    for (int j = 0; j < NUM_CORES; ++j) {
        Cache* o = b->caches[j];
        if (o == c) { // skip the requester
            continue;
        }
        shared |= cache_bus_rdx(o, addr);
    }
    if (shared) ++b->invalidations;            // 1 per transaction
    c->total_cycles += shared ? c2c_cycles(c) : LOAD_MEM_CYCLES;
    b->data_traffic += c->block_size;
    b->t_free = c->total_cycles;
}

