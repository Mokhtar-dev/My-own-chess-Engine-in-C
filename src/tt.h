#ifndef TT_H
#define TT_H

#include <stdint.h>

#define EXACT 0
#define ALPHA 1
#define BETA 2

typedef struct {
    uint64_t key;
    int depth;
    int flag;
    int score;
} TTEntry;

void tt_init();
void tt_store(uint64_t key, int depth, int flag, int score);
int tt_probe(uint64_t key, int depth, int alpha, int beta, int* score);

#endif