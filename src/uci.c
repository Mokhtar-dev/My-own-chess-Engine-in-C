#include "uci.h"       /* own header — always first */
#include "board.h"     /* Board, board_from_fen     */
#include "movegen.h"   /* generate_moves            */
#include "search.h"    /* search                    */
#include <stdio.h>     /* printf, scanf             */
#include <string.h>    /* strcmp, strncmp           */