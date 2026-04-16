#ifndef SEARCH_H
#define SEARCH_H

#include "board.h"

#define INF 1000000 /* effectively infinity */

// node counter (for debugging)
extern int nodes;

// main search function
int search_position(Board* board, int depth, int* best_move);

// alpha-beta
int alpha_beta(Board* board, int depth, int alpha, int beta);

#endif