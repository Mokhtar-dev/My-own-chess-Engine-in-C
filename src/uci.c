#include "uci.h" /* own header — always first */

#include <stdio.h> /* printf, scanf             */
#include <stdlib.h>
#include <string.h> /* strcmp, strncmp           */

#include "board.h"   /* Board, board_from_fen     */
#include "movegen.h" /* generate_moves            */
#include "search.h"  /* search                    */
#include "utils.h"   /* move_to_string            */

void uci_loop() {
    char input[512];
    Board board;

    board_start(&board);
    fflush(stdout);

    while (fgets(input, sizeof(input), stdin) != NULL) {
        // Remove newline if present
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n')
            input[len - 1] = '\0';

        if (strcmp(input, "uci") == 0) {
            printf("id name MokhtarEngine\n");
            printf("id author Mokhtar\n");
            printf("option name Hash type spin default 16 min 1 max 512\n");
            printf("uciok\n");
            fflush(stdout);
        } else if (strcmp(input, "isready") == 0) {
            printf("readyok\n");
            fflush(stdout);
        } else if (strcmp(input, "ucinewgame") == 0) {
            board_start(&board);
        } else if (strncmp(input, "position startpos", 17) == 0) {
            board_start(&board);
            /* parse moves if present */
            char* moves_ptr = strstr(input, "moves");
            if (moves_ptr) {
                moves_ptr += 6; /* skip "moves " */
                char move_str[10];
                while (sscanf(moves_ptr, "%s", move_str) == 1) {
                    int move = string_to_move(&board, move_str);
                    if (move == 0) break; /* invalid move */
                    UndoInfo undo;
                    make_move(&board, move, &undo);
                    moves_ptr = strchr(moves_ptr, ' ');
                    if (moves_ptr == NULL) break;
                    moves_ptr++;
                }
            }
        } else if (strncmp(input, "position fen", 12) == 0) {
            char fen[256];
            strncpy(fen, input + 13, sizeof(fen) - 1);
            fen[sizeof(fen) - 1] = '\0';

            /* find "moves" keyword and truncate fen at that point */
            char* moves_ptr = strstr(fen, "moves");
            if (moves_ptr) {
                *(moves_ptr - 1) = '\0'; /* remove " moves" part from fen */
            }

            board_from_fen(&board, fen);

            /* parse moves if present */
            if (moves_ptr) {
                moves_ptr += 6; /* skip "moves " */
                char move_str[10];
                while (sscanf(moves_ptr, "%s", move_str) == 1) {
                    int move = string_to_move(&board, move_str);
                    if (move == 0) break; /* invalid move */
                    UndoInfo undo;
                    make_move(&board, move, &undo);
                    moves_ptr = strchr(moves_ptr, ' ');
                    if (moves_ptr == NULL) break;
                    moves_ptr++;
                }
            }
        } else if (strncmp(input, "go", 2) == 0) {
            int best_move;
            search_position(&board, 4, &best_move);
            char move_str[6];
            move_to_string(best_move, move_str);
            printf("bestmove %s\n", move_str);
            fflush(stdout);
        } else if (strcmp(input, "quit") == 0) {
            break;
        }
    }
}