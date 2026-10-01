#ifndef CC_H
#define CC_H

#define NUM_CORES 1
#define WORD_SIZE 4

#define ACCESS_CYCLES 1
#define FLUSH_MEM_CYCLES 100
#define LOAD_MEM_CYCLES 100
#define C2C_CYCLES_PER_WORD 2

#ifdef DEBUG
    #define LOG(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#else
    #define LOG(fmt, ...)
#endif

#endif
