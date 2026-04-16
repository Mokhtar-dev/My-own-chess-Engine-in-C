#include "eval.h"
#include "board.h"
#include "utils.h"
#include <stdint.h>

/* -------------------------------------------------------
   Piece-square tables (from White's perspective)
   Values are bonuses in centipawns added to piece value.
   Index 0 = a1, index 63 = h8.
   Black uses the same table but mirrored vertically.
------------------------------------------------------- */

/* mirror: flip rank for black */
#define MIRROR(sq) ((sq) ^ 56)

static const int pst_pawn[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
     5,  5, 10, 25, 25, 10,  5,  5,
     0,  0,  0, 20, 20,  0,  0,  0,
     5, -5,-10,  0,  0,-10, -5,  5,
     5, 10, 10,-20,-20, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0,
};

static const int pst_knight[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50,
};

static const int pst_bishop[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20,
};

static const int pst_rook[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
     5, 10, 10, 10, 10, 10, 10,  5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     0,  0,  0,  5,  5,  0,  0,  0,
};

static const int pst_queen[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
     -5,  0,  5,  5,  5,  5,  0, -5,
      0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20,
};

/* king middlegame — stay safe behind pawns */
static const int pst_king_mg[64] = {
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
     20, 20,  0,  0,  0,  0, 20, 20,
     20, 30, 10,  0,  0, 10, 30, 20,
};

/* -------------------------------------------------------
   piece_value — base material value by piece index
------------------------------------------------------- */
static const int piece_value[12] = {
    VAL_PAWN, VAL_KNIGHT, VAL_BISHOP, VAL_ROOK, VAL_QUEEN, VAL_KING,
    VAL_PAWN, VAL_KNIGHT, VAL_BISHOP, VAL_ROOK, VAL_QUEEN, VAL_KING,
};

/* -------------------------------------------------------
   pst_score — piece-square bonus for one piece on sq
------------------------------------------------------- */
static int pst_score(int piece, int sq) {
    /* white pieces: use table directly
       black pieces: mirror the square vertically        */
    int s = (piece >= BP) ? MIRROR(sq) : sq;

    switch (piece) {
        case WP: case BP: return pst_pawn[s];
        case WN: case BN: return pst_knight[s];
        case WB: case BB: return pst_bishop[s];
        case WR: case BR: return pst_rook[s];
        case WQ: case BQ: return pst_queen[s];
        case WK: case BK: return pst_king_mg[s];
        default:          return 0;
    }
}

/* -------------------------------------------------------
   eval — static evaluation of position
   Returns score in centipawns, positive = good for
   the side to move.
------------------------------------------------------- */
int eval(const Board *b) {
    int score = 0;

    for (int p = 0; p < 12; p++) {
        uint64_t bb = b->pieces[p];
        while (bb) {
            int sq = get_lsb(bb);
            CLEAR_BIT(bb, sq);

            int val = piece_value[p] + pst_score(p, sq);

            /* white pieces add, black pieces subtract */
            if (p < BP)
                score += val;
            else
                score -= val;
        }
    }

    /* return from the perspective of the side to move */
    return (b->side == WHITE) ? score : -score;
}