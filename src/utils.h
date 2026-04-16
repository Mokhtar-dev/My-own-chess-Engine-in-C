#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

#include "board.h"
#include "movegen.h"

/* returns index of the least significant set bit */
int get_lsb(uint64_t bb);

/* counts number of set bits */
int popcount(uint64_t bb);

/* converts move integer to algebraic notation (e.g., "e2e4") */
void move_to_string(int move, char* str);

/* finds a move from algebraic notation in the legal moves (e.g., "e2e4")
   returns 0 if not found */
int string_to_move(Board* board, const char* str);

#endif