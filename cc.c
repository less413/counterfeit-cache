#include <stdio.h>
#include <stdlib.h>

int main(int ac, char** av) {
    if (ac < 3 || ac > 6) {
        printf("\nUsage: %s <protocol> <input_file> [<cache_size> [<associativity> [<block_size>]]]\n\n", av[0]);
        return -1;
    }

    char* protocol = av[1];
    char* input_file = av[2];
    int cache_size = 4096;
    int associativity = 2;
    int block_size = 32;
    
    if (ac >= 4) {
        cache_size = atoi(av[3]);
    }
    
    if (ac >= 5) {
        associativity = atoi(av[4]);
    }
    

    if (ac >= 6) {
        block_size = atoi(av[5]);
    }
    
    printf("\n");
    printf("protocol: %s\n", protocol);
    printf("input_file: %s\n", input_file);
    printf("cache_size: %d\n", cache_size);
    printf("associativity: %d\n", associativity);
    printf("block_size: %d\n", block_size);
    printf("\n");
}

