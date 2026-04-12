#include "utils.h"

int get_lsb(uint64_t bb) {
    /* uses GCC built-in — counts trailing zeros = index of lowest set bit */
    return __builtin_ctzll(bb);
}

int popcount(uint64_t bb) {
    return __builtin_popcountll(bb);
}