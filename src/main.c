#include "board.h"     /* board_init, board_start   */
#include "movegen.h"   /* movegen_init              */
#include "uci.h"       /* uci_loop                  */
#include <stdio.h>

int main(void) {
    board_init();
    movegen_init();

    Board b;
    board_start(&b);
    board_print(&b);

    /* test a custom position */
    board_from_fen(&b,"r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4");
    board_print(&b);

    return 0;
}