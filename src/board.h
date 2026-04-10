#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>

/* -------------------------------------------------------
   Piece indices — used to index into pieces[12]
   0-5 = white, 6-11 = black
------------------------------------------------------- */
#define WP 0  /* white pawn   */
#define WN 1  /* white knight */
#define WB 2  /* white bishop */
#define WR 3  /* white rook   */
#define WQ 4  /* white queen  */
#define WK 5  /* white king   */
#define BP 6  /* black pawn   */
#define BN 7  /* black knight */
#define BB 8  /* black bishop */
#define BR 9  /* black rook   */
#define BQ 10 /* black queen  */
#define BK 11 /* black king   */

/* -------------------------------------------------------
   Sides
------------------------------------------------------- */
#define WHITE 0
#define BLACK 1
#define BOTH 2

/* -------------------------------------------------------
   Castling rights — stored as 4 bits
------------------------------------------------------- */
#define WKS 1 /* white kingside  */
#define WQS 2 /* white queenside */
#define BKS 4 /* black kingside  */
#define BQS 8 /* black queenside */

/* -------------------------------------------------------
   Square definitions (a1=0 ... h8=63)
------------------------------------------------------- */
#define A1 0 #define B1 1 #define C1 2 #define D1 3
#define E1 4 #define F1 5 #define G1 6 #define H1 7

#define A2 8 #define B2 9 #define C2 10 #define D2 11
#define E2 12 #define F2 13 #define G2 14 #define H2 15

#define A3 16 #define B3 17 #define C3 18 #define D3 19
#define E3 20 #define F3 21 #define G3 22 #define H3 23

#define A4 24 #define B4 25 #define C4 26 #define D4 27
#define E4 28 #define F4 29 #define G4 30 #define H4 31

#define A5 32 #define B5 33 #define C5 34 #define D5 35
#define E5 36 #define F5 37 #define G5 38 #define H5 39

#define A6 40 #define B6 41 #define C6 42 #define D6 43
#define E6 44 #define F6 45 #define G6 46 #define H6 47

#define A7 48 #define B7 49 #define C7 50 #define D7 51
#define E7 52 #define F7 53 #define G7 54 #define H7 55

#define A8 56 #define B8 57 #define C8 58 #define D8 59
#define E8 60 #define F8 61 #define G8 62 #define H8 63

#define NO_SQ -1

/* -------------------------------------------------------
   Helper macros
------------------------------------------------------- */
#define GET_BIT(bb, sq) (((bb) >> (sq)) & 1ULL)
#define SET_BIT(bb, sq) ((bb) |= (1ULL << (sq)))
#define CLEAR_BIT(bb, sq) ((bb) &= ~(1ULL << (sq)))

#define RANK(sq) ((sq) >> 3) /* sq / 8 */
#define FILE(sq) ((sq) & 7)  /* sq % 8 */

/* -------------------------------------------------------
   Board state
------------------------------------------------------- */
typedef struct {
    uint64_t pieces[12];   /* one bitboard per piece type  */
    uint64_t occupancy[3]; /* [WHITE], [BLACK], [BOTH]     */

    int side;            /* WHITE or BLACK to move       */
    int en_passant;      /* en passant target sq, or NO_SQ */
    int castling;        /* 4-bit castling rights mask   */
    int halfmove_clock;  /* for 50-move rule             */
    int fullmove_number; /* increments after black moves */

    uint64_t zobrist_key; /* for transposition table      */
} Board;

/* -------------------------------------------------------
   Zobrist tables — declared here, defined in board.c
------------------------------------------------------- */
extern uint64_t zobrist_pieces[12][64];
extern uint64_t zobrist_side;
extern uint64_t zobrist_castling[16];
extern uint64_t zobrist_ep[8];

/* -------------------------------------------------------
   Function prototypes
------------------------------------------------------- */
void board_init(void);                          /* call once at startup   */
void board_start(Board* b);                     /* load starting position */
void board_from_fen(Board* b, const char* fen); /* load any FEN string    */
void board_print(const Board* b);               /* print to terminal      */
void board_update_occupancy(Board* b);          /* rebuild occupancy[]    */
uint64_t board_compute_zobrist(const Board* b); /* recompute key from scratch */

#endif