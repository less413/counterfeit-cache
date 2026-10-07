#ifndef PARSE_H
#define PARSE_H

#include <stdint.h>
#include <stdlib.h>

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

#endif
