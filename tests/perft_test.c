#include "board.h"     /* Board, make_move, undo    */
#include "movegen.h"   /* MoveList, generate_moves  */
#include <stdio.h>

long long perft(Board* b, int depth) {
    if (depth == 0) return 1;

    MoveList list;
    generate_moves(b, &list);
    long long nodes = 0;

    for (int i = 0; i < list.count; i++) {
        UndoInfo undo;
        if (make_move(b, list.moves[i], &undo)) {
            nodes += perft(b, depth - 1);
            undo_move(b, list.moves[i], &undo);
        }
    }
    return nodes;
}

int main(void) {
    board_init();
    movegen_init();

    Board b;
    board_start(&b);

    printf("Perft results from starting position:\n\n");

    for (int depth = 1; depth <= 5; depth++) {
        long long nodes = perft(&b, depth);
        printf("  depth %d: %lld\n", depth, nodes);
    }

    return 0;
}