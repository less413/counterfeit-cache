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
static ulong c2c_cycles(Cache* c) {
    return C2C_CYCLES_PER_WORD * (c->block_size / WORD_SIZE);
}

// BusRd  (exclusive = 0): E/S/M -> S   (M flushes; memory picks it up too)
// BusRdX (exclusive = 1): E/S/M -> I   (int exclusive flag to indicate BusRdX)
// return 1 if other cache has a copy, else 0
static int snoop(Cache* c, Bus* b, uint addr, int exclusive) {
    int found = 0;
    if (exclusive) {
        for (int j = 0; j < NUM_CORES; ++j) {
            Cache* o = b->caches[j];
            if (o == c) { // skip the requester
                continue;
            }
            found |= cache_bus_rdx(o, addr);
        }
    } else {
        for (int j = 0; j < NUM_CORES; ++j) {
            Cache* o = b->caches[j];
            if (o == c) { // skip the requester
                continue;
            }
            found |= cache_bus_rd(o, addr);
        }
    }
    return found;
}

// BusRd for a read miss. Returns the requester's new state: S if shared, else E.
uint bus_rd(Cache* c, Bus* b, uint addr) {
    int shared = snoop(c, b, addr, 0);
    c->total_cycles += shared ? c2c_cycles(c) : LOAD_MEM_CYCLES;
    b->data_traffic += c->block_size;
    b->t_free = c->total_cycles;
    return shared ? ST_S : ST_E;
}

// BusRdX for a write miss, or a write hit in S. Requester ends in M.
void bus_rdx(Cache* c, Bus* b, uint addr) {
    int shared = snoop(c, b, addr, 1);
    if (shared) ++b->invalidations;            // 1 per transaction
    c->total_cycles += shared ? c2c_cycles(c) : LOAD_MEM_CYCLES;
    b->data_traffic += c->block_size;
    b->t_free = c->total_cycles;
}

