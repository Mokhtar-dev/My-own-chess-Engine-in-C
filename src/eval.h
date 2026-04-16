#ifndef EVAL_H
#define EVAL_H

#include "board.h"

/* -------------------------------------------------------
   Score constants
------------------------------------------------------- */
#define INF        1000000   /* effectively infinity         */
#define MATE_SCORE  100000   /* base mate score              */

/* -------------------------------------------------------
   Piece values (centipawns)
------------------------------------------------------- */
#define VAL_PAWN   100
#define VAL_KNIGHT 320
#define VAL_BISHOP 330
#define VAL_ROOK   500
#define VAL_QUEEN  900
#define VAL_KING     0   /* king has no material value */

/* -------------------------------------------------------
   Function prototypes
------------------------------------------------------- */

/* Full static evaluation — positive = good for side to move */
int eval(const Board *b);

#endif