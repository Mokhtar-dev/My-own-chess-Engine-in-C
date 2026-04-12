#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "board.h"

/* -------------------------------------------------------
   Move list — holds all generated moves for one position
------------------------------------------------------- */
#define MAX_MOVES 256

typedef struct {
    int moves[MAX_MOVES];
    int count;
} MoveList;

/* -------------------------------------------------------
   Attack tables — precomputed at startup
------------------------------------------------------- */
extern uint64_t pawn_attacks[2][64]; /* [side][square] */
extern uint64_t knight_attacks[64];
extern uint64_t king_attacks[64];

/* -------------------------------------------------------
   Function prototypes
------------------------------------------------------- */
void movegen_init(void);
int is_square_attacked(const Board* b, int sq, int by_side);
void generate_moves(const Board* b, MoveList* list);

/* helper — add a move to the list */
static inline void add_move(MoveList* list, int move) {
    list->moves[list->count++] = move;
}

#endif