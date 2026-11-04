#include <stdint.h>

#include "cc.h"
#include "cache.h"
#include "bus.h"
#include "mesif.h"

static uint64_t mesif_supply_cycles(Cache* c, int shared, int dirty, int forwarded) {
    if (dirty) {
        // M owner flushes once; requester (2N) and memory (100) receive it together
        uint64_t c2c = c2c_cycles(c);
        return c2c > FLUSH_MEM_CYCLES ? c2c : FLUSH_MEM_CYCLES;
    }
    if (forwarded) {
        // F owner supplies cache-to-cache (2N), but not to memory
        return c2c_cycles(c);
    }
    // not dirty: any S or E states dont provide c2c
    return LOAD_MEM_CYCLES;
}

// BusRd for a read miss. Returns the requester's new state: F if shared, else E.
uint32_t mesif_bus_rd(Cache* c, Bus* b, uint32_t addr) {
    int shared = 0;
    int dirty = 0;
    int forwarded = 0;
    for (int j = 0; j < NUM_CORES; ++j) {
        Cache* o = b->caches[j];
        if (o == c) { // skip the requester
            continue;
        }
        int old = mesif_cache_bus_rd(o, addr);
        shared |= (old != MESIF_I);
        dirty |= (old == MESIF_M);
        forwarded |= (old == MESIF_F);
    }
    c->total_cycles += mesif_supply_cycles(c, shared, dirty, forwarded);
    b->data_traffic += c->block_size;
    b->t_free = c->total_cycles;
    return shared ? MESIF_F : MESIF_E;
}

// BusRdX for a write miss, or a write hit in S. Requester ends in M.
void mesif_bus_rdx(Cache* c, Bus* b, uint32_t addr) {
    int shared = 0;
    int dirty = 0;
    int forwarded = 0;
    for (int j = 0; j < NUM_CORES; ++j) {
        Cache* o = b->caches[j];
        if (o == c) { // skip the requester
            continue;
        }
        int old = mesif_cache_bus_rdx(o, addr);
        shared |= (old != MESIF_I);
        dirty |= (old == MESIF_M);
        forwarded |= (old == MESIF_F);
    }
    if (shared) ++b->invalidations;            // 1 per transaction
    c->total_cycles += mesif_supply_cycles(c, shared, dirty, forwarded);
    b->data_traffic += c->block_size;
    b->t_free = c->total_cycles;
}

