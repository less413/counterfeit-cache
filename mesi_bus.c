#include <stdint.h>

#include "cc.h"
#include "cache.h"
#include "bus.h"
#include "mesi.h"

static uint64_t mesi_supply_cycles(Cache* c, int dirty) {
    if (dirty) {
        // M owner flushes once; requester (2N) and memory (100) receive it together
        uint64_t c2c = c2c_cycles(c);
        return c2c > FLUSH_MEM_CYCLES ? c2c : FLUSH_MEM_CYCLES;
    }
    // not dirty: any S or E states dont provide c2c
    return LOAD_MEM_CYCLES;
}

// BusRd for a read miss. Returns the requester's new state: S if shared, else E.
uint32_t mesi_bus_rd(Cache* c, Bus* b, uint32_t addr) {
    int shared = 0;
    int dirty = 0;
    for (int j = 0; j < NUM_CORES; ++j) {
        Cache* o = b->caches[j];
        if (o == c) { // skip the requester
            continue;
        }
        int old = mesi_cache_bus_rd(o, addr);
        shared |= (old != ST_I);
        dirty |= (old == ST_M);
    }
    c->total_cycles += mesi_supply_cycles(c, dirty);
    b->data_traffic += c->block_size;
    b->t_free = c->total_cycles;
    return shared ? ST_S : ST_E;
}

// BusRdX for a write miss, or a write hit in S. Requester ends in M.
void mesi_bus_rdx(Cache* c, Bus* b, uint32_t addr) {
    int shared = 0;
    int dirty = 0;
    for (int j = 0; j < NUM_CORES; ++j) {
        Cache* o = b->caches[j];
        if (o == c) { // skip the requester
            continue;
        }
        int old = mesi_cache_bus_rdx(o, addr);
        shared |= (old != ST_I);
        dirty |= (old == ST_M);
    }
    if (shared) ++b->invalidations;            // 1 per transaction
    c->total_cycles += mesi_supply_cycles(c, dirty);
    b->data_traffic += c->block_size;
    b->t_free = c->total_cycles;
}

