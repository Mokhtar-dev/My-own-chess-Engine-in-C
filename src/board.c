#include "board.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------
   Zobrist tables
------------------------------------------------------- */
uint64_t zobrist_pieces[12][64];
uint64_t zobrist_side;
uint64_t zobrist_castling[16];
uint64_t zobrist_ep[8];

/* simple pseudo-random 64-bit number generator (xorshift64) */
static uint64_t rng_state = 1070372631ULL;

static uint64_t random_u64(void) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 7;
    rng_state ^= rng_state << 17;
    return rng_state;
}

/* -------------------------------------------------------
   board_init — call once at program startup
------------------------------------------------------- */
void board_init(void) {
    /* fill every zobrist table with random 64-bit values */
    for (int p = 0; p < 12; p++)
        for (int sq = 0; sq < 64; sq++)
            zobrist_pieces[p][sq] = random_u64();

    zobrist_side = random_u64();

    for (int i = 0; i < 16; i++)
        zobrist_castling[i] = random_u64();

    for (int i = 0; i < 8; i++)
        zobrist_ep[i] = random_u64();
}

/* -------------------------------------------------------
   board_update_occupancy — rebuild from pieces[]
------------------------------------------------------- */
void board_update_occupancy(Board *b) {
    b->occupancy[WHITE] = 0ULL;
    b->occupancy[BLACK] = 0ULL;

    for (int p = WP; p <= WK; p++) b->occupancy[WHITE] |= b->pieces[p];
    for (int p = BP; p <= BK; p++) b->occupancy[BLACK] |= b->pieces[p];

    b->occupancy[BOTH] = b->occupancy[WHITE] | b->occupancy[BLACK];
}

/* -------------------------------------------------------
   board_compute_zobrist — build key from scratch
   (call after loading a position; update incrementally
    in make_move / undo_move during search)
------------------------------------------------------- */
uint64_t board_compute_zobrist(const Board *b) {
    uint64_t key = 0ULL;

    for (int p = 0; p < 12; p++) {
        uint64_t bb = b->pieces[p];
        while (bb) {
            int sq = get_lsb(bb);          /* from utils.h */
            key ^= zobrist_pieces[p][sq];
            CLEAR_BIT(bb, sq);
        }
    }

    if (b->side == BLACK)
        key ^= zobrist_side;

    key ^= zobrist_castling[b->castling];

    if (b->en_passant != NO_SQ)
        key ^= zobrist_ep[FILE(b->en_passant)];

    return key;
}

/* -------------------------------------------------------
   board_from_fen — parse a FEN string into the board
------------------------------------------------------- */
void board_from_fen(Board *b, const char *fen) {
    memset(b, 0, sizeof(Board));
    b->en_passant = NO_SQ;

    /* --- piece placement --- */
    int sq = 56;   /* start at a8, go left-to-right, top-to-bottom */
    const char *ptr = fen;

    while (*ptr && *ptr != ' ') {
        if (*ptr == '/') {
            sq -= 16;   /* drop a rank, reset to left */
        } else if (*ptr >= '1' && *ptr <= '8') {
            sq += (*ptr - '0');
        } else {
            /* map FEN character to piece index */
            const char *pieces = "PNBRQKpnbrqk";
            const char *found = strchr(pieces, *ptr);
            if (found) {
                int piece = (int)(found - pieces);
                SET_BIT(b->pieces[piece], sq);
                sq++;
            }
        }
        ptr++;
    }

    /* --- side to move --- */
    ptr++;   /* skip space */
    b->side = (*ptr == 'w') ? WHITE : BLACK;
    ptr += 2;

    /* --- castling rights --- */
    b->castling = 0;
    while (*ptr && *ptr != ' ') {
        switch (*ptr) {
            case 'K': b->castling |= WKS; break;
            case 'Q': b->castling |= WQS; break;
            case 'k': b->castling |= BKS; break;
            case 'q': b->castling |= BQS; break;
        }
        ptr++;
    }
    ptr++;

    /* --- en passant square --- */
    if (*ptr != '-') {
        int file = ptr[0] - 'a';
        int rank = ptr[1] - '1';
        b->en_passant = rank * 8 + file;
        ptr += 2;
    } else {
        b->en_passant = NO_SQ;
        ptr++;
    }
    ptr++;

    /* --- halfmove clock --- */
    b->halfmove_clock = atoi(ptr);
    while (*ptr && *ptr != ' ') ptr++;
    ptr++;

    /* --- fullmove number --- */
    b->fullmove_number = atoi(ptr);

    /* --- finalize --- */
    board_update_occupancy(b);
    b->zobrist_key = board_compute_zobrist(b);
}

/* -------------------------------------------------------
   board_start — load the standard starting position
------------------------------------------------------- */
void board_start(Board *b) {
    board_from_fen(b,
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

/* -------------------------------------------------------
   board_print — pretty-print to terminal
------------------------------------------------------- */
void board_print(const Board *b) {
    const char *symbols[] = {
        "P","N","B","R","Q","K",
        "p","n","b","r","q","k"
    };

    printf("\n");
    for (int rank = 7; rank >= 0; rank--) {
        printf("  %d  ", rank + 1);
        for (int file = 0; file < 8; file++) {
            int sq = rank * 8 + file;
            int found = -1;
            for (int p = 0; p < 12; p++) {
                if (GET_BIT(b->pieces[p], sq)) {
                    found = p;
                    break;
                }
            }
            printf(" %s", found >= 0 ? symbols[found] : ".");
        }
        printf("\n");
    }
    printf("\n     a b c d e f g h\n\n");
    printf("  Side:     %s\n", b->side == WHITE ? "white" : "black");
    printf("  Castling: %c%c%c%c\n",
        (b->castling & WKS) ? 'K' : '-',
        (b->castling & WQS) ? 'Q' : '-',
        (b->castling & BKS) ? 'k' : '-',
        (b->castling & BQS) ? 'q' : '-');
    printf("  En passant: %s\n",
        b->en_passant != NO_SQ ? "yes" : "none");
    printf("  Zobrist: %llx\n\n",
        (unsigned long long)b->zobrist_key);
}

int piece_on(const Board *b, int sq) {
    for (int p = 0; p < 12; p++)
        if (GET_BIT(b->pieces[p], sq)) return p;
    return -1;   /* empty square */
}


