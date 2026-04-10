#include <stdio.h>
#include "board.h"

int main(void) {
    board_init();

    Board b;
    board_start(&b);
    board_print(&b);

    /* test a custom position */
    board_from_fen(&b,"r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4");
    board_print(&b);

    return 0;
}