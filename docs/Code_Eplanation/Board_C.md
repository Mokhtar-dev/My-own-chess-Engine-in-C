# Board.c Code Explanation

I'll break down your chess engine's board module piece by piece.

## **Zobrist Hashing Tables (Lines 6-13)**

```c
uint64_t zobrist_pieces[12][64];
uint64_t zobrist_side;
uint64_t zobrist_castling[16];
uint64_t zobrist_ep[8];
```

These are **lookup tables for Zobrist hashing** — a technique to create unique fingerprints of board positions:
- `zobrist_pieces[12][64]` — Random numbers for each of 12 piece types on each of 64 squares
- `zobrist_side` — Random number toggled when side changes
- `zobrist_castling[16]` — Random numbers for 16 possible castling combinations
- `zobrist_ep[8]` — Random numbers for en passant on each file (a-h)

## **Random Number Generator (Lines 15-22)**

```c
static uint64_t rng_state = 1070372631ULL;

static uint64_t random_u64(void) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 7;
    rng_state ^= rng_state << 17;
    return rng_state;
}
```

**XORshift64** — A fast pseudo-random number generator. It performs three bitwise XOR operations with shifts to produce unpredictable 64-bit numbers for the Zobrist tables.

## **board_init() (Lines 26-39)**

```c
void board_init(void) {
    for (int p = 0; p < 12; p++)
        for (int sq = 0; sq < 64; sq++)
            zobrist_pieces[p][sq] = random_u64();
    zobrist_side = random_u64();
    for (int i = 0; i < 16; i++)
        zobrist_castling[i] = random_u64();
    for (int i = 0; i < 8; i++)
        zobrist_ep[i] = random_u64();
}
```

**Initialize all Zobrist tables** with random values. **Call this once at program startup.**

## **board_update_occupancy() (Lines 43-51)**

```c
void board_update_occupancy(Board *b) {
    b->occupancy[WHITE] = 0ULL;
    b->occupancy[BLACK] = 0ULL;
    for (int p = WP; p <= WK; p++) b->occupancy[WHITE] |= b->pieces[p];
    for (int p = BP; p <= BK; p++) b->occupancy[BLACK] |= b->pieces[p];
    b->occupancy[BOTH] = b->occupancy[WHITE] | b->occupancy[BLACK];
}
```

**Rebuilds occupancy bitboards** — combines all piece bitboards into three summary bitboards:
- White pieces, Black pieces, and Both combined (all pieces)
- Uses bitwise OR (`|=`) to merge all pieces together

## **board_compute_zobrist() (Lines 55-72)**

```c
uint64_t board_compute_zobrist(const Board *b) {
    uint64_t key = 0ULL;
    for (int p = 0; p < 12; p++) {
        uint64_t bb = b->pieces[p];
        while (bb) {
            int sq = get_lsb(bb);          
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
```

**Creates a unique hash of the board position** by XORing random numbers:
- Loop through each piece type and square
- XOR the corresponding Zobrist value for each piece
- XOR side-to-move, castling rights, and en passant file
- Result: fast position comparison for transposition tables

## **board_from_fen() (Lines 76-134)**

**Parses a FEN string** (Forsyth-Edwards Notation) into the board structure:

1. **Piece placement** (ranks 8→1, files a→h)
   - '/' separates ranks
   - Digits skip empty squares
   - Letters place pieces
   
2. **Side to move** — 'w' or 'b'

3. **Castling rights** — K/Q/k/q flags

4. **En passant square** — e.g., "e3" or "-"

5. **Halfmove and fullmove clocks** — for 50-move rule and move numbering

Finally, it updates occupancy and computes the Zobrist key.

## **board_start() (Lines 138-142)**

```c
void board_start(Board *b) {
    board_from_fen(b,
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}
```

**Loads the standard chess starting position** using the standard starting FEN string.

## **board_print() (Lines 146-180)**

**Pretty-prints the board to the terminal** with:
- 8x8 grid showing all pieces
- Rank numbers (1-8) and file letters (a-h)
- Board metadata: side to move, castling rights, en passant, Zobrist key

## **piece_on() (Lines 182-186)**

```c
int piece_on(const Board *b, int sq) {
    for (int p = 0; p < 12; p++)
        if (GET_BIT(b->pieces[p], sq)) return p;
    return -1;
}
```

**Returns which piece is on a square**, or -1 if empty. Used for quick piece lookup.

---

**Key takeaway:** This module manages the board representation using **bitboards** (64-bit integers) for efficiency, **Zobrist hashing** for fast position comparison, and **FEN parsing** for loading chess positions.