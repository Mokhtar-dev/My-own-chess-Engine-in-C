#include "search.h"    /* own header — always first */
#include "board.h"     /* Board, make_move, undo    */
#include "movegen.h"   /* MoveList, generate_moves  */
#include "eval.h"      /* eval                      */
#include "tt.h"        /* tt_probe, tt_store        */