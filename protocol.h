#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#include "cache.h"
#include "bus.h"

typedef struct Protocol {
    void (*pr_rd)(Cache* c, Bus* b, uint32_t addr);
    void (*pr_wr)(Cache* c, Bus* b, uint32_t addr);
    int (*needs_bus)(Cache* c, uint32_t addr, int is_write);
} Protocol;

#endif

