#ifndef CC_H
#define CC_H

#include <stdio.h>

#define NUM_CORES 4
#define WORD_SIZE 4

#define ACCESS_CYCLES 1
#define FLUSH_MEM_CYCLES 100
#define LOAD_MEM_CYCLES 100
#define C2C_CYCLES_PER_WORD 2

// 0 = invalid, 1 = shared, 2 = exclusive, 3 = modified
enum { ST_I = 0, ST_S = 1, ST_E = 2, ST_M = 3 };

#ifdef DEBUG
    #define LOG(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#else
    #define LOG(fmt, ...)
#endif

#endif
