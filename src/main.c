#include <stdio.h>

#include "board.h"   /* board_init, board_start   */
#include "movegen.h" /* movegen_init              */
#include "uci.h"     /* uci_loop                  */

int main(void) {
    board_init();
    movegen_init();

    uci_loop();

    return 0;
}