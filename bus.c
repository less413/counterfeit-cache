#include <stdlib.h>
#include "cc.h"
#include "bus.h"


Bus* bus_create(Cache** caches) {
    Bus* b = malloc(sizeof(Bus));

    // initialize
    b->data_traffic = 0;
    b->invalidations = 0;
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
}

void bus_rd(Cache* c, Bus* b) {
    c->total_cycles += LOAD_MEM_CYCLES;
    b->data_traffic += c->block_size;
}
