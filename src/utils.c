#include "utils.h"

#include <stdio.h>
#include <string.h>

#include "board.h"
#include "movegen.h"

int get_lsb(uint64_t bb) {
    /* uses GCC built-in — counts trailing zeros = index of lowest set bit */
    return __builtin_ctzll(bb);
}

int popcount(uint64_t bb) {
    return __builtin_popcountll(bb);
}

/* converts move integer to algebraic notation (e.g., "e2e4") */
void move_to_string(int move, char* str) {
    int from = GET_FROM(move);
    int to = GET_TO(move);
    int promo = GET_PROMO(move);

    char from_file = 'a' + (from % 8);
    char from_rank = '1' + (from / 8);
    char to_file = 'a' + (to % 8);
    char to_rank = '1' + (to / 8);

    if (promo != 0) {
        const char* promo_chars = "nbrq";
        int promo_type = (promo >> 4) & 3; /* get promotion piece type */
        sprintf(str, "%c%c%c%c%c", from_file, from_rank, to_file, to_rank,
                promo_type < 4 ? promo_chars[promo_type] : ' ');
    } else {
        sprintf(str, "%c%c%c%c", from_file, from_rank, to_file, to_rank);
    }
}

/* finds a move from algebraic notation in the legal moves (e.g., "e2e4")
   returns 0 if not found */
int string_to_move(Board* board, const char* str) {
    /* parse algebraic notation: e2e4 = from(2 chars) + to(2 chars) [+ promo(1 char)] */
    if (strlen(str) < 4)
        return 0;

    int from_file = str[0] - 'a';
    int from_rank = str[1] - '1';
    int to_file = str[2] - 'a';
    int to_rank = str[3] - '1';

    /* validate square ranges */
    if (from_file < 0 || from_file > 7 || from_rank < 0 || from_rank > 7)
        return 0;
    if (to_file < 0 || to_file > 7 || to_rank < 0 || to_rank > 7)
        return 0;

    int from_sq = from_rank * 8 + from_file;
    int to_sq = to_rank * 8 + to_file;

    /* generate all legal moves */
    MoveList list;
    list.count = 0;
    generate_moves(board, &list);

    /* find matching move */
    for (int i = 0; i < list.count; i++) {
        int move = list.moves[i];
        if (GET_FROM(move) == from_sq && GET_TO(move) == to_sq) {
            /* if promotion move, check if promo piece matches */
            int promo = GET_PROMO(move);
            if (strlen(str) > 4 && promo != 0) {
                /* promotion specified in notation */
                const char* promo_chars = "nbrq";
                char promo_char = str[4];
                int promo_idx = -1;
                for (int j = 0; j < 4; j++) {
                    if (promo_chars[j] == promo_char) {
                        promo_idx = j;
                        break;
                    }
                }
                int move_promo_type = (promo >> 4) & 3;
                if (promo_idx >= 0 && promo_idx == move_promo_type)
                    return move;
            } else if (strlen(str) == 4 && promo == 0) {
                /* non-promotion move */
                return move;
            }
        }
    }

    return 0;
}