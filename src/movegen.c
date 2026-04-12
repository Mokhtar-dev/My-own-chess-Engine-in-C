#include "movegen.h"   /* own header — always first */
#include "board.h"     /* Board, macros, piece defs, square defs */
#include "utils.h"     /* get_lsb, popcount */
#include <stdio.h>
/* -------------------------------------------------------
   Attack tables — filled once in movegen_init()
------------------------------------------------------- */
uint64_t pawn_attacks[2][64];
uint64_t knight_attacks[64];
uint64_t king_attacks[64];

/* -------------------------------------------------------
   Sliding piece attack generators (ray-based)
   Walk in a direction until hitting a blocker or edge
------------------------------------------------------- */

/* bishop: diagonals (NE, NW, SE, SW) */
static uint64_t bishop_attacks_otf(int sq, uint64_t blockers) {
    uint64_t attacks = 0ULL;
    int r = RANK(sq), f = FILE(sq);
    int tr, tf;

    /* north-east */
    for (tr = r+1, tf = f+1; tr < 8 && tf < 8; tr++, tf++) {
        attacks |= (1ULL << (tr*8 + tf));
        if (blockers & (1ULL << (tr*8 + tf))) break;
    }
    /* north-west */
    for (tr = r+1, tf = f-1; tr < 8 && tf >= 0; tr++, tf--) {
        attacks |= (1ULL << (tr*8 + tf));
        if (blockers & (1ULL << (tr*8 + tf))) break;
    }
    /* south-east */
    for (tr = r-1, tf = f+1; tr >= 0 && tf < 8; tr--, tf++) {
        attacks |= (1ULL << (tr*8 + tf));
        if (blockers & (1ULL << (tr*8 + tf))) break;
    }
    /* south-west */
    for (tr = r-1, tf = f-1; tr >= 0 && tf >= 0; tr--, tf--) {
        attacks |= (1ULL << (tr*8 + tf));
        if (blockers & (1ULL << (tr*8 + tf))) break;
    }
    return attacks;
}

/* rook: straight lines (N, S, E, W) */
static uint64_t rook_attacks_otf(int sq, uint64_t blockers) {
    uint64_t attacks = 0ULL;
    int r = RANK(sq), f = FILE(sq);
    int tr, tf;

    /* north */
    for (tr = r+1; tr < 8; tr++) {
        attacks |= (1ULL << (tr*8 + f));
        if (blockers & (1ULL << (tr*8 + f))) break;
    }
    /* south */
    for (tr = r-1; tr >= 0; tr--) {
        attacks |= (1ULL << (tr*8 + f));
        if (blockers & (1ULL << (tr*8 + f))) break;
    }
    /* east */
    for (tf = f+1; tf < 8; tf++) {
        attacks |= (1ULL << (r*8 + tf));
        if (blockers & (1ULL << (r*8 + tf))) break;
    }
    /* west */
    for (tf = f-1; tf >= 0; tf--) {
        attacks |= (1ULL << (r*8 + tf));
        if (blockers & (1ULL << (r*8 + tf))) break;
    }
    return attacks;
}

/* queen = bishop + rook combined */
static uint64_t queen_attacks_otf(int sq, uint64_t blockers) {
    return bishop_attacks_otf(sq, blockers) |
           rook_attacks_otf(sq, blockers);
}

/* -------------------------------------------------------
   movegen_init — precompute all attack tables
   Call once at startup before any move generation
------------------------------------------------------- */
void movegen_init(void) {
    /* --- pawn attacks --- */
    for (int sq = 0; sq < 64; sq++) {
        uint64_t bb = 1ULL << sq;
        int r = RANK(sq), f = FILE(sq);

        /* white pawns attack diagonally upward */
        pawn_attacks[WHITE][sq] = 0ULL;
        if (r < 7 && f > 0) pawn_attacks[WHITE][sq] |= (1ULL << (sq + 7));
        if (r < 7 && f < 7) pawn_attacks[WHITE][sq] |= (1ULL << (sq + 9));

        /* black pawns attack diagonally downward */
        pawn_attacks[BLACK][sq] = 0ULL;
        if (r > 0 && f > 0) pawn_attacks[BLACK][sq] |= (1ULL << (sq - 9));
        if (r > 0 && f < 7) pawn_attacks[BLACK][sq] |= (1ULL << (sq - 7));

        (void)bb;
    }

    /* --- knight attacks --- */
    /* all 8 possible L-shaped jumps */
    int knight_offsets[8][2] = {
        {-2,-1},{-2,1},{-1,-2},{-1,2},
        { 2,-1},{ 2,1},{ 1,-2},{ 1,2}
    };
    for (int sq = 0; sq < 64; sq++) {
        knight_attacks[sq] = 0ULL;
        int r = RANK(sq), f = FILE(sq);
        for (int i = 0; i < 8; i++) {
            int tr = r + knight_offsets[i][0];
            int tf = f + knight_offsets[i][1];
            if (tr >= 0 && tr < 8 && tf >= 0 && tf < 8)
                knight_attacks[sq] |= (1ULL << (tr*8 + tf));
        }
    }

    /* --- king attacks --- */
    /* all 8 surrounding squares */
    int king_offsets[8][2] = {
        {-1,-1},{-1,0},{-1,1},
        { 0,-1},       { 0,1},
        { 1,-1},{ 1,0},{ 1,1}
    };
    for (int sq = 0; sq < 64; sq++) {
        king_attacks[sq] = 0ULL;
        int r = RANK(sq), f = FILE(sq);
        for (int i = 0; i < 8; i++) {
            int tr = r + king_offsets[i][0];
            int tf = f + king_offsets[i][1];
            if (tr >= 0 && tr < 8 && tf >= 0 && tf < 8)
                king_attacks[sq] |= (1ULL << (tr*8 + tf));
        }
    }
}

/* -------------------------------------------------------
   is_square_attacked — is sq attacked by by_side?
   Used by make_move (legality) and castling checks
------------------------------------------------------- */
int is_square_attacked(const Board *b, int sq, int by_side) {
    /* pawns */
    int our_pawn = (by_side == WHITE) ? WP : BP;
    if (pawn_attacks[by_side ^ 1][sq] & b->pieces[our_pawn]) return 1;

    /* knights */
    int our_knight = (by_side == WHITE) ? WN : BN;
    if (knight_attacks[sq] & b->pieces[our_knight]) return 1;

    /* bishops / queens (diagonal) */
    int our_bishop = (by_side == WHITE) ? WB : BB;
    int our_queen  = (by_side == WHITE) ? WQ : BQ;
    if (bishop_attacks_otf(sq, b->occupancy[BOTH]) &
        (b->pieces[our_bishop] | b->pieces[our_queen])) return 1;

    /* rooks / queens (straight) */
    int our_rook = (by_side == WHITE) ? WR : BR;
    if (rook_attacks_otf(sq, b->occupancy[BOTH]) &
        (b->pieces[our_rook] | b->pieces[our_queen])) return 1;

    /* king */
    int our_king = (by_side == WHITE) ? WK : BK;
    if (king_attacks[sq] & b->pieces[our_king]) return 1;

    return 0;
}

/* -------------------------------------------------------
   generate_moves — fill list with all pseudo-legal moves
   make_move() filters illegal ones (king left in check)
------------------------------------------------------- */
void generate_moves(const Board *b, MoveList *list) {
    list->count = 0;
    int side  = b->side;
    int enemy = side ^ 1;

    /* piece indices for current side */
    int pawn   = (side == WHITE) ? WP : BP;
    int knight = (side == WHITE) ? WN : BN;
    int bishop = (side == WHITE) ? WB : BB;
    int rook   = (side == WHITE) ? WR : BR;
    int queen  = (side == WHITE) ? WQ : BQ;
    int king   = (side == WHITE) ? WK : BK;

    uint64_t our_pieces   = b->occupancy[side];
    uint64_t their_pieces = b->occupancy[enemy];
    uint64_t all_pieces   = b->occupancy[BOTH];
    uint64_t empty        = ~all_pieces;

    /* --- PAWNS ---------------------------------------- */
    {
        uint64_t bb = b->pieces[pawn];
        while (bb) {
            int from = get_lsb(bb);
            CLEAR_BIT(bb, from);
            int r = RANK(from);

            if (side == WHITE) {
                /* single push */
                int to = from + 8;
                if (to < 64 && GET_BIT(empty, to)) {
                    if (r == 6) {
                        /* promotion */
                        add_move(list, ENCODE_MOVE(from,to,pawn,WQ,0,0,0,0));
                        add_move(list, ENCODE_MOVE(from,to,pawn,WR,0,0,0,0));
                        add_move(list, ENCODE_MOVE(from,to,pawn,WB,0,0,0,0));
                        add_move(list, ENCODE_MOVE(from,to,pawn,WN,0,0,0,0));
                    } else {
                        add_move(list, ENCODE_MOVE(from,to,pawn,0,0,0,0,0));
                        /* double push from rank 2 */
                        if (r == 1 && GET_BIT(empty, from+16))
                            add_move(list, ENCODE_MOVE(from,from+16,pawn,0,0,1,0,0));
                    }
                }
                /* captures */
                uint64_t caps = pawn_attacks[WHITE][from] & their_pieces;
                while (caps) {
                    int to2 = get_lsb(caps);
                    CLEAR_BIT(caps, to2);
                    if (r == 6) {
                        add_move(list, ENCODE_MOVE(from,to2,pawn,WQ,1,0,0,0));
                        add_move(list, ENCODE_MOVE(from,to2,pawn,WR,1,0,0,0));
                        add_move(list, ENCODE_MOVE(from,to2,pawn,WB,1,0,0,0));
                        add_move(list, ENCODE_MOVE(from,to2,pawn,WN,1,0,0,0));
                    } else {
                        add_move(list, ENCODE_MOVE(from,to2,pawn,0,1,0,0,0));
                    }
                }
                /* en passant */
                if (b->en_passant != NO_SQ &&
                    (pawn_attacks[WHITE][from] & (1ULL << b->en_passant)))
                    add_move(list, ENCODE_MOVE(from,b->en_passant,pawn,0,1,0,1,0));

            } else {
                /* single push */
                int to = from - 8;
                if (to >= 0 && GET_BIT(empty, to)) {
                    if (r == 1) {
                        /* promotion */
                        add_move(list, ENCODE_MOVE(from,to,pawn,BQ,0,0,0,0));
                        add_move(list, ENCODE_MOVE(from,to,pawn,BR,0,0,0,0));
                        add_move(list, ENCODE_MOVE(from,to,pawn,BB,0,0,0,0));
                        add_move(list, ENCODE_MOVE(from,to,pawn,BN,0,0,0,0));
                    } else {
                        add_move(list, ENCODE_MOVE(from,to,pawn,0,0,0,0,0));
                        /* double push from rank 7 */
                        if (r == 6 && GET_BIT(empty, from-16))
                            add_move(list, ENCODE_MOVE(from,from-16,pawn,0,0,1,0,0));
                    }
                }
                /* captures */
                uint64_t caps = pawn_attacks[BLACK][from] & their_pieces;
                while (caps) {
                    int to2 = get_lsb(caps);
                    CLEAR_BIT(caps, to2);
                    if (r == 1) {
                        add_move(list, ENCODE_MOVE(from,to2,pawn,BQ,1,0,0,0));
                        add_move(list, ENCODE_MOVE(from,to2,pawn,BR,1,0,0,0));
                        add_move(list, ENCODE_MOVE(from,to2,pawn,BB,1,0,0,0));
                        add_move(list, ENCODE_MOVE(from,to2,pawn,BN,1,0,0,0));
                    } else {
                        add_move(list, ENCODE_MOVE(from,to2,pawn,0,1,0,0,0));
                    }
                }
                /* en passant */
                if (b->en_passant != NO_SQ &&
                    (pawn_attacks[BLACK][from] & (1ULL << b->en_passant)))
                    add_move(list, ENCODE_MOVE(from,b->en_passant,pawn,0,1,0,1,0));
            }
        }
    }

    /* --- KNIGHTS -------------------------------------- */
    {
        uint64_t bb = b->pieces[knight];
        while (bb) {
            int from = get_lsb(bb);
            CLEAR_BIT(bb, from);
            uint64_t attacks = knight_attacks[from] & ~our_pieces;
            while (attacks) {
                int to = get_lsb(attacks);
                CLEAR_BIT(attacks, to);
                int cap = GET_BIT(their_pieces, to) ? 1 : 0;
                add_move(list, ENCODE_MOVE(from,to,knight,0,cap,0,0,0));
            }
        }
    }

    /* --- BISHOPS -------------------------------------- */
    {
        uint64_t bb = b->pieces[bishop];
        while (bb) {
            int from = get_lsb(bb);
            CLEAR_BIT(bb, from);
            uint64_t attacks = bishop_attacks_otf(from, all_pieces) & ~our_pieces;
            while (attacks) {
                int to = get_lsb(attacks);
                CLEAR_BIT(attacks, to);
                int cap = GET_BIT(their_pieces, to) ? 1 : 0;
                add_move(list, ENCODE_MOVE(from,to,bishop,0,cap,0,0,0));
            }
        }
    }

    /* --- ROOKS ---------------------------------------- */
    {
        uint64_t bb = b->pieces[rook];
        while (bb) {
            int from = get_lsb(bb);
            CLEAR_BIT(bb, from);
            uint64_t attacks = rook_attacks_otf(from, all_pieces) & ~our_pieces;
            while (attacks) {
                int to = get_lsb(attacks);
                CLEAR_BIT(attacks, to);
                int cap = GET_BIT(their_pieces, to) ? 1 : 0;
                add_move(list, ENCODE_MOVE(from,to,rook,0,cap,0,0,0));
            }
        }
    }

    /* --- QUEENS --------------------------------------- */
    {
        uint64_t bb = b->pieces[queen];
        while (bb) {
            int from = get_lsb(bb);
            CLEAR_BIT(bb, from);
            uint64_t attacks = queen_attacks_otf(from, all_pieces) & ~our_pieces;
            while (attacks) {
                int to = get_lsb(attacks);
                CLEAR_BIT(attacks, to);
                int cap = GET_BIT(their_pieces, to) ? 1 : 0;
                add_move(list, ENCODE_MOVE(from,to,queen,0,cap,0,0,0));
            }
        }
    }

    /* --- KING ----------------------------------------- */
    {
        int from = get_lsb(b->pieces[king]);
        uint64_t attacks = king_attacks[from] & ~our_pieces;
        while (attacks) {
            int to = get_lsb(attacks);
            CLEAR_BIT(attacks, to);
            int cap = GET_BIT(their_pieces, to) ? 1 : 0;
            add_move(list, ENCODE_MOVE(from,to,king,0,cap,0,0,0));
        }

        /* castling */
        if (side == WHITE) {
            /* kingside: squares F1 and G1 must be empty and not attacked */
            if ((b->castling & WKS) &&
                !GET_BIT(all_pieces, F1) &&
                !GET_BIT(all_pieces, G1) &&
                !is_square_attacked(b, E1, BLACK) &&
                !is_square_attacked(b, F1, BLACK) &&
                !is_square_attacked(b, G1, BLACK))
                add_move(list, ENCODE_MOVE(E1,G1,king,0,0,0,0,1));

            /* queenside: squares B1, C1, D1 empty; E1, D1, C1 not attacked */
            if ((b->castling & WQS) &&
                !GET_BIT(all_pieces, D1) &&
                !GET_BIT(all_pieces, C1) &&
                !GET_BIT(all_pieces, B1) &&
                !is_square_attacked(b, E1, BLACK) &&
                !is_square_attacked(b, D1, BLACK) &&
                !is_square_attacked(b, C1, BLACK))
                add_move(list, ENCODE_MOVE(E1,C1,king,0,0,0,0,1));

        } else {
            /* black kingside */
            if ((b->castling & BKS) &&
                !GET_BIT(all_pieces, F8) &&
                !GET_BIT(all_pieces, G8) &&
                !is_square_attacked(b, E8, WHITE) &&
                !is_square_attacked(b, F8, WHITE) &&
                !is_square_attacked(b, G8, WHITE))
                add_move(list, ENCODE_MOVE(E8,G8,king,0,0,0,0,1));

            /* black queenside */
            if ((b->castling & BQS) &&
                !GET_BIT(all_pieces, D8) &&
                !GET_BIT(all_pieces, C8) &&
                !GET_BIT(all_pieces, B8) &&
                !is_square_attacked(b, E8, WHITE) &&
                !is_square_attacked(b, D8, WHITE) &&
                !is_square_attacked(b, C8, WHITE))
                add_move(list, ENCODE_MOVE(E8,C8,king,0,0,0,0,1));
        }
    }
}