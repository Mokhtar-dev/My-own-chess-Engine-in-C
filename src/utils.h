#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

/* returns index of the least significant set bit */
int get_lsb(uint64_t bb);

/* counts number of set bits */
int popcount(uint64_t bb);

#endif