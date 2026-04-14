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

/* -------------------------------------------------------
   make_move — apply move to the board
   Returns 1 if legal (king not left in check), 0 if illegal
------------------------------------------------------- */
int make_move(Board *b, int move, UndoInfo *undo) {
    int from   = GET_FROM(move);
    int to     = GET_TO(move);
    int piece  = GET_PIECE(move);
    int promo  = GET_PROMO(move);
    int is_cap = IS_CAPTURE(move);
    int is_dp  = IS_DOUBLE(move);
    int is_ep  = IS_EP(move);
    int is_cas = IS_CASTLE(move);
 
    /* --- save undo info --- */
    undo->castling       = b->castling;
    undo->en_passant     = b->en_passant;
    undo->halfmove_clock = b->halfmove_clock;
    undo->zobrist_key    = b->zobrist_key;
    undo->captured       = -1;
 
    /* --- update zobrist: remove old state --- */
    b->zobrist_key ^= zobrist_castling[b->castling];
    if (b->en_passant != NO_SQ)
        b->zobrist_key ^= zobrist_ep[FILE(b->en_passant)];
    b->zobrist_key ^= zobrist_side;
 
    /* --- halfmove clock --- */
    if (is_cap || piece == WP || piece == BP)
        b->halfmove_clock = 0;
    else
        b->halfmove_clock++;
 
    /* --- handle capture --- */
    if (is_cap && !is_ep) {
        int captured = piece_on(b, to);
        undo->captured = captured;
        CLEAR_BIT(b->pieces[captured], to);
        b->zobrist_key ^= zobrist_pieces[captured][to];
    }
 
    /* --- handle en passant capture --- */
    if (is_ep) {
        int ep_sq = (b->side == WHITE) ? to - 8 : to + 8;
        int captured = (b->side == WHITE) ? BP : WP;
        undo->captured = captured;
        CLEAR_BIT(b->pieces[captured], ep_sq);
        b->zobrist_key ^= zobrist_pieces[captured][ep_sq];
    }
 
    /* --- move the piece --- */
    CLEAR_BIT(b->pieces[piece], from);
    b->zobrist_key ^= zobrist_pieces[piece][from];
 
    if (promo) {
        SET_BIT(b->pieces[promo], to);
        b->zobrist_key ^= zobrist_pieces[promo][to];
    } else {
        SET_BIT(b->pieces[piece], to);
        b->zobrist_key ^= zobrist_pieces[piece][to];
    }
 
    /* --- update en passant square --- */
    b->en_passant = NO_SQ;
    if (is_dp) {
        b->en_passant = (b->side == WHITE) ? to - 8 : to + 8;
        b->zobrist_key ^= zobrist_ep[FILE(b->en_passant)];
    }
 
    /* --- handle castling: move the rook --- */
    if (is_cas) {
        int rook  = (b->side == WHITE) ? WR : BR;
        int rfrom, rto;
        if (to == G1) { rfrom = H1; rto = F1; }
        else if (to == C1) { rfrom = A1; rto = D1; }
        else if (to == G8) { rfrom = H8; rto = F8; }
        else               { rfrom = A8; rto = D8; }
 
        CLEAR_BIT(b->pieces[rook], rfrom);
        SET_BIT(b->pieces[rook], rto);
        b->zobrist_key ^= zobrist_pieces[rook][rfrom];
        b->zobrist_key ^= zobrist_pieces[rook][rto];
    }
 
    /* --- update castling rights --- */
    /* any move from/to these squares forfeits the right */
    const int castling_mask[64] = {
        [A1] = ~WQS, [E1] = ~(WKS|WQS), [H1] = ~WKS,
        [A8] = ~BQS, [E8] = ~(BKS|BQS), [H8] = ~BKS,
    };
    /* default mask for unaffected squares is 0xF (keep all) */
    int mask_from = (from < 64 && castling_mask[from]) ? castling_mask[from] : 0xF;
    int mask_to   = (to   < 64 && castling_mask[to])   ? castling_mask[to]   : 0xF;
    b->castling &= (mask_from & mask_to);
    b->zobrist_key ^= zobrist_castling[b->castling];
 
    /* --- switch side --- */
    b->side ^= 1;
    if (b->side == WHITE) b->fullmove_number++;
 
    /* --- rebuild occupancy --- */
    board_update_occupancy(b);
 
    /* --- legality check: own king must not be in check --- */
    int king_piece = (b->side == WHITE) ? BK : WK; /* side already flipped */
    int king_sq    = get_lsb(b->pieces[king_piece]);
    if (is_square_attacked(b, king_sq, b->side)) {
        undo_move(b, move, undo);
        return 0;
    }
 
    return 1;
}
 
/* -------------------------------------------------------
   undo_move — restore board to state before make_move
------------------------------------------------------- */
void undo_move(Board *b, int move, const UndoInfo *undo) {
    int from   = GET_FROM(move);
    int to     = GET_TO(move);
    int piece  = GET_PIECE(move);
    int promo  = GET_PROMO(move);
    int is_ep  = IS_EP(move);
    int is_cas = IS_CASTLE(move);
 
    /* --- switch side back --- */
    b->side ^= 1;
    if (b->side == BLACK) b->fullmove_number--;
 
    /* --- move piece back --- */
    if (promo) {
        CLEAR_BIT(b->pieces[promo], to);
    } else {
        CLEAR_BIT(b->pieces[piece], to);
    }
    SET_BIT(b->pieces[piece], from);
 
    /* --- restore captured piece --- */
    if (undo->captured != -1 && !is_ep) {
        SET_BIT(b->pieces[undo->captured], to);
    }
 
    /* --- restore en passant captured pawn --- */
    if (is_ep && undo->captured != -1) {
        int ep_sq = (b->side == WHITE) ? to - 8 : to + 8;
        SET_BIT(b->pieces[undo->captured], ep_sq);
    }
 
    /* --- undo castling rook move --- */
    if (is_cas) {
        int rook  = (b->side == WHITE) ? WR : BR;
        int rfrom, rto;
        if (to == G1) { rfrom = H1; rto = F1; }
        else if (to == C1) { rfrom = A1; rto = D1; }
        else if (to == G8) { rfrom = H8; rto = F8; }
        else               { rfrom = A8; rto = D8; }
 
        CLEAR_BIT(b->pieces[rook], rto);
        SET_BIT(b->pieces[rook], rfrom);
    }
 
    /* --- restore saved state --- */
    b->castling       = undo->castling;
    b->en_passant     = undo->en_passant;
    b->halfmove_clock = undo->halfmove_clock;
    b->zobrist_key    = undo->zobrist_key;
 
    board_update_occupancy(b);
}


