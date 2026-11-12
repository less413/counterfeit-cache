#include <stdint.h>

#include "cc.h"
#include "cache.h"
#include "bus.h"
#include "dragon.h"

// #cycles to supply the block
static uint64_t dragon_supply_cycles(Cache* c, int dirty) {
    // only M/Sm supplies cache-to-cache, while E/Sc don't
    return dirty ? c2c_cycles(c) : LOAD_MEM_CYCLES;
}

// updates other E/M copies to Sc/Sm state
// returns 1 if other copies exist, 0 otherwise
// i might change this to return non-zero instead for simplicity
int dragon_bus_rd(Cache *c, Bus *b, uint32_t addr) {
    int shared = 0; // tracks bitwise OR of other copies' states
    for (int j = 0; j < NUM_CORES; ++j) {
        Cache* o = b->caches[j];
        if (o == c) { // skip the requestor
            continue;
        }
        shared |= dragon_cache_bus_rd(o, addr);
    }
    // if shared != 0, means there is shared copy elsewhere
    // if (shared & 2) != 0, means there is a dirty copy elsewhere
    int dirty = shared & 2;
    c->total_cycles += dragon_supply_cycles(c, dirty);
    b->data_traffic += c->block_size;
    b->t_free = c->total_cycles;
    return shared != 0;
}

// updates other copies to Sc state
// returns 1 if other copies exist, 0 otherwise
// i might change this to return non-zero instead for simplicity
int dragon_bus_upd(Cache *c, Bus *b, uint32_t addr) {
    int shared = 0; // tracks bitwise OR of other copies' states
    for (int j = 0; j <NUM_CORES; ++j) {
        Cache* o = b->caches[j];
        if (o == c) { // skip the requestor
            continue;
        }
        shared |= dragon_cache_bus_upd(o, addr);
    }
    if (shared) ++b->invalidations; // 1 per transaction
    c->total_cycles += C2C_CYCLES_PER_WORD; // BusUpd sends out 1 word
    b->data_traffic += WORD_SIZE;
    b->t_free = c->total_cycles;
    return shared != 0;
}

