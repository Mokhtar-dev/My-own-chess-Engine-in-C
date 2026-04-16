#include <stdio.h>

#include "uci.h"

int main() {
    fprintf(stderr, "Engine starting...\n");
    fflush(stderr);
    uci_loop();
    fprintf(stderr, "Engine stopped\n");
    fflush(stderr);
    return 0;
}