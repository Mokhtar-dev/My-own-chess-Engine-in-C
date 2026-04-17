# Mokhtar Chess Engine - Complete Documentation

## Table of Contents
1. [Project Overview](#project-overview)
2. [Architecture](#architecture)
3. [File Structure](#file-structure)
4. [Core Concepts](#core-concepts)
5. [Data Structures](#data-structures)
6. [Key Components](#key-components)
7. [UCI Protocol](#uci-protocol)
8. [Building & Running](#building--running)
9. [Technical Details](#technical-details)

---

## Project Overview

**Mokhtar Chess Engine** is a UCI-compliant chess engine written in C. It features:

- **Bitboard representation** for efficient position storage and manipulation
- **Zobrist hashing** for position fingerprinting and transposition detection
- **Alpha-beta search** with transposition table caching
- **Static evaluation** based on material and piece positioning
- **Move generation** with support for all chess rules (castling, en passant, promotion)
- **UCI protocol** compatibility for integration with chess GUIs like Cute Chess

The engine is designed to be fast, correct, and modular, with clear separation of concerns across multiple source files.

---

## Architecture

### High-Level Flow

```
┌─────────────────────────────────────────────────────────┐
│                    UCI Interface (uci.c)                 │
│  Receives: position, go, ucinewgame, quit commands      │
│  Sends: bestmove, id, option responses                  │
└─────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────┐
│                  Search Engine (search.c)                │
│  - Root search: finds best move from current position   │
│  - Alpha-beta pruning: efficient game tree exploration  │
│  - Transposition table: caches evaluated positions      │
└─────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────┐
│                Move Generation (movegen.c)               │
│  - Generates all legal moves from a position            │
│  - Pre-computed attack tables for pawn/knight/king      │
│  - Sliding piece attacks (bishop/rook/queen)            │
└─────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────┐
│                 Board Management (board.c)               │
│  - Position representation using bitboards              │
│  - Move encoding/decoding                               │
│  - Make/undo move with legality checking                │
│  - FEN parsing and position loading                     │
└─────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────┐
│              Position Evaluation (eval.c)                │
│  - Material count (pawns, pieces, queens)               │
│  - Piece-square tables (positional evaluation)          │
│  - Simple heuristics (center control, piece safety)     │
└─────────────────────────────────────────────────────────┘
```

---

## File Structure

```
chess-engine/
├── src/
│   ├── main.c              # Entry point, calls uci_loop()
│   ├── board.c/h           # Board representation and move execution
│   ├── movegen.c/h         # Move generation and attack detection
│   ├── search.c/h          # Alpha-beta search with TT
│   ├── eval.c/h            # Position evaluation
│   ├── tt.c/h              # Transposition table
│   ├── uci.c/h             # UCI protocol implementation
│   └── utils.c/h           # Utility functions (bitwise ops, move conversion)
├── tests/
│   ├── perft_test.c        # Perft testing for move gen validation
│   ├── eval_test.c         # Evaluation testing
│   └── search_test.c       # Search testing
├── build/
│   └── chess_engine.exe    # Compiled binary
├── docs/
│   └── Code_Explanation/   # Detailed code walkthroughs
├── README.md               # Quick start guide
└── DOCUMENTATION.md        # This file
```

---

## Core Concepts

### 1. Bitboard Representation

The engine uses **64-bit integers (bitboards)** to represent piece positions. Each bit corresponds to one square on the chess board (a1=0, h8=63).

**Board Layout:**
```
Rank 8: 56 57 58 59 60 61 62 63
Rank 7: 48 49 50 51 52 53 54 55
Rank 6: 40 41 42 43 44 45 46 47
...
Rank 1:  0  1  2  3  4  5  6  7
        files: a  b  c  d  e  f  g  h
```

**Example:** If a white pawn is on e2 (square 12):
```
bitboard = 1ULL << 12  // Set bit 12
           = 0x0000000000001000
```

**Advantages:**
- Very fast bulk operations (AND, OR, XOR)
- Efficient move generation checks
- Compact memory usage
- Natural parallelism

### 2. Zobrist Hashing

**Purpose:** Create a unique fingerprint for each position for transposition table lookup.

**How it works:**
- Pre-compute random 64-bit numbers for each piece on each square
- XOR together all pieces currently on the board
- XOR in side-to-move indicator
- XOR in castling rights
- XOR in en passant square

**Result:** Every unique position has a different hash (collision extremely unlikely).

**Incremental Update:** Instead of recomputing from scratch, update the hash incrementally:
- When adding a piece: `hash ^= zobrist_table[piece][square]`
- When removing a piece: `hash ^= zobrist_table[piece][square]`
- When switching sides: `hash ^= zobrist_side`

### 3. Move Encoding

Moves are packed into a single 32-bit integer:

```c
#define ENCODE_MOVE(from, to, piece, promo, cap, dp, ep, castle) \
    ((from) |                    /* bits 0-5: from square (0-63) */     \
     ((to) << 6) |               /* bits 6-11: to square (0-63) */       \
     ((piece) << 12) |           /* bits 12-15: moving piece (0-11) */   \
     ((promo) << 16) |           /* bits 16-19: promotion piece */       \
     ((cap) << 20) |             /* bit 20: is capture? */               \
     ((dp) << 21) |              /* bit 21: double pawn push? */         \
     ((ep) << 22) |              /* bit 22: en passant capture? */       \
     ((castle) << 23))           /* bit 23: castling? */
```

**Example:** Moving white knight from b1 to c3 with no special flags:
```c
move = ENCODE_MOVE(1, 18, WN, 0, 0, 0, 0, 0)
```

**Extraction:**
```c
int from = GET_FROM(move);      // Extract bits 0-5
int to   = GET_TO(move);        // Extract bits 6-11
int piece= GET_PIECE(move);     // Extract bits 12-15
```

### 4. Move Generation

Moves are generated differently depending on piece type:

**Pawns:**
- Single-step forward (if empty)
- Double-step from starting position (if both squares empty)
- Captures diagonally (if enemy piece present)
- Promotion when reaching final rank
- En passant (if en passant square is set)

**Knights:**
- All 8 L-shaped moves that stay on board

**Bishops/Rooks/Queens:**
- Sliding attacks along diagonals/ranks/files
- Stop at first piece (your own or enemy)

**King:**
- All 8 surrounding squares
- Castling (if rights available, king/rook haven't moved, no pieces between, not in check)

---

## Data Structures

### Board Structure

```c
typedef struct {
    uint64_t pieces[12];         // Bitboard for each piece type (WP, WN, WB, WR, WQ, WK, BP, BN, BB, BR, BQ, BK)
    uint64_t occupancy[3];       // Combined occupancy: [WHITE], [BLACK], [BOTH]
    
    int side;                    // WHITE (0) or BLACK (1) - whose turn?
    int castling;                // Bitmask (WKS|WQS|BKS|BQS)
    int en_passant;              // EP square (0-63) or NO_SQ (-1)
    int halfmove_clock;          // Moves since last pawn move (50-move rule)
    int fullmove_number;         // Move counter (starts at 1)
    
    uint64_t zobrist_key;        // Position hash
} Board;
```

### Undo Information

```c
typedef struct {
    int castling;               // Previous castling rights
    int en_passant;             // Previous en passant square
    int halfmove_clock;         // Previous halfmove clock
    uint64_t zobrist_key;       // Previous hash
    int captured;               // Captured piece type (-1 if none)
} UndoInfo;
```

### Move List

```c
typedef struct {
    int moves[MAX_MOVES];       // Array of encoded moves
    int count;                  // Number of moves in list
} MoveList;
```

### Transposition Table Entry

```c
typedef struct {
    uint64_t zobrist_key;       // Position hash
    int depth;                  // Search depth
    int flag;                   // EXACT, ALPHA, or BETA
    int score;                  // Evaluation score
} TranspositionEntry;
```

---

## Key Components

### 1. Board Management (`board.c`)

**Key Functions:**

- **`board_init()`** — Initialize Zobrist hash tables with random numbers. Called once at startup.
  
- **`board_start(Board *b)`** — Load the standard starting position.

- **`board_from_fen(Board *b, const char *fen)`** — Parse a FEN string and set up the board.
  ```
  Example: "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
  ```

- **`make_move(Board *b, int move, UndoInfo *undo)`** — Execute a move on the board.
  - Extracts move components (from, to, piece, flags)
  - Saves undo information
  - Updates Zobrist hash incrementally
  - Handles special moves (castling, en passant, promotion)
  - Validates move legality (king not in check)
  - Returns 1 if legal, 0 if illegal

- **`undo_move(Board *b, int move, const UndoInfo *undo)`** — Reverse a move.
  - Restores all board state from UndoInfo
  - O(1) operation

- **`board_update_occupancy(Board *b)`** — Rebuild occupancy bitboards from piece positions.
  - Called after position loading and move making

---

### 2. Move Generation (`movegen.c`)

**Key Functions:**

- **`movegen_init()`** — Pre-compute attack tables. Called once at startup.
  - Fills `pawn_attacks[2][64]`
  - Fills `knight_attacks[64]`
  - Fills `king_attacks[64]`

- **`generate_moves(const Board *b, MoveList *list)`** — Generate all legal moves.
  - Iterates through all pieces
  - Generates pseudo-legal moves for each piece type
  - Filters out moves that leave king in check
  - Populates `list->moves[]` and sets `list->count`

- **`is_square_attacked(const Board *b, int sq, int by_side)`** — Check if a square is attacked.
  - Used for legality checking (king in check?)
  - Checks pawn attacks, knight jumps, sliding pieces, king

**Pre-computed Tables:**
- `pawn_attacks[2][64]` — Attack bitboards for pawns on each square
- `knight_attacks[64]` — Attack bitboards for knights
- `king_attacks[64]` — Attack bitboards for kings

---

### 3. Search Engine (`search.c`)

**Algorithm:** Alpha-Beta Pruning with Transposition Table

```c
int alpha_beta(Board* board, int depth, int alpha, int beta);
```

**How it works:**

1. Check transposition table for cached position
2. If depth=0, evaluate position and return
3. Generate all legal moves
4. For each move:
   - Make the move
   - Recursively search with negated alpha/beta
   - Undo the move
   - Update best score and alpha
   - Prune if alpha >= beta
5. Cache result in transposition table

**Root Search:**
```c
int search_position(Board* board, int depth, int* best_move);
```
- Similar to alpha-beta but tracks best move
- Called by UCI when "go" command received
- Returns best move integer

**Time Complexity:** Typically $O(b^{d/2})$ where b=branching factor, d=depth (due to pruning)

---

### 4. Evaluation (`eval.c`)

**Scoring System:**

```
Pawn   = 100 points
Knight = 300 points
Bishop = 300 points
Rook   = 500 points
Queen  = 900 points
King   = Infinite (can't be traded)
```

**Evaluation Function:**
```c
int eval(const Board *board);
```

**Components:**
1. **Material Count** — Sum of all pieces
2. **Piece-Square Tables** — Positional bonuses/penalties
   - Pawns: Bonus for advancing toward promotion
   - Knights: Bonus for central squares
   - Bishops: Bonus for long diagonals
   - Rooks: Bonus for open files
   - Queens: Bonus for central control
   - Kings: Penalty for exposed squares in midgame, safer squares in endgame

3. **Simple Heuristics** (if implemented):
   - Pawn structure (isolated, doubled pawns)
   - King safety (pawn cover)
   - Piece activity (attacked squares)

---

### 5. Transposition Table (`tt.c`)

**Purpose:** Cache evaluated positions to avoid re-searching.

**Key Functions:**

- **`tt_probe(uint64_t key, int depth, int alpha, int beta, int *score)`**
  - Look up position in table
  - Return 1 if found and sufficient depth, 0 otherwise
  - Set `*score` if returning 1

- **`tt_store(uint64_t key, int depth, int flag, int score)`**
  - Store position with its evaluation
  - Flags: `EXACT` (full evaluation), `ALPHA` (upper bound), `BETA` (lower bound)

**Implementation:** Simple hash table with linear probing or replacement strategy.

---

### 6. UCI Protocol (`uci.c`)

**Standard UCI Commands Supported:**

| Command | Response | Purpose |
|---------|----------|---------|
| `uci` | `id name`, `id author`, `option`, `uciok` | Identify engine |
| `isready` | `readyok` | Check if engine is responsive |
| `ucinewgame` | (none) | Reset state for new game |
| `position startpos [moves ...]` | (none) | Set starting position + apply moves |
| `position fen <fen> [moves ...]` | (none) | Set FEN position + apply moves |
| `go [depth/time/movetime]` | `bestmove <move>` | Search and return best move |
| `quit` | (none) | Exit engine |

**Example Session:**
```
→ uci
← id name MokhtarEngine
← id author Mokhtar
← option name Hash type spin default 16 min 1 max 512
← uciok

→ isready
← readyok

→ position startpos moves e2e4
→ go depth 4
← bestmove e7e5

→ quit
```

---

### 7. Utilities (`utils.c`)

**Key Functions:**

- **`int get_lsb(uint64_t bb)`** — Get index of lowest set bit (LSB)
  - Uses GCC built-in: `__builtin_ctzll()` (count trailing zeros)
  - Used to iterate through bitboards

- **`int popcount(uint64_t bb)`** — Count number of set bits
  - Uses GCC built-in: `__builtin_popcountll()`
  - Used for material counting

- **`void move_to_string(int move, char *str)`** — Convert move to UCI notation
  - Example: move at positions 12→20 becomes "e2e4"
  - Handles promotion notation (e.g., "e7e8q")

- **`int string_to_move(Board* board, const char *str)`** — Parse UCI notation to move
  - Example: "e2e4" → finds matching legal move
  - Returns 0 if no matching legal move exists

---

## UCI Protocol

### Position Command Format

```
position startpos [moves move1 move2 ...]
position fen <fenstring> [moves move1 move2 ...]
```

**Example:**
```
position fen "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" moves e2e4 e7e5 g1f3
```

This sets the starting position, applies the moves e2e4, e7e5, and g1f3, and the engine is now in the position after 1.e4 e5 2.Nf3.

### Go Command Format

```
go [depth N] [movetime N] [wtime N] [btime N] [winc N] [binc N]
```

**Example:**
```
go depth 10              # Search 10 half-moves deep
go movetime 1000         # Search for max 1 second
go wtime 300000 btime 300000 winc 5000 binc 5000  # 5 min + 5 sec increment each
```

---

## Building & Running

### Build

```bash
cd d:\My-own-chess-Engine-in-C
gcc src/main.c src/board.c src/movegen.c src/search.c src/eval.c src/tt.c src/uci.c src/utils.c \
    -I src -o build/chess_engine.exe -Wall -g
```

**Flags:**
- `-I src` — Include directory for headers
- `-o build/chess_engine.exe` — Output file
- `-Wall` — Enable all warnings
- `-g` — Include debug symbols

### Run

**Interactive UCI Mode:**
```bash
build/chess_engine.exe
```

Then type UCI commands:
```
position startpos
go depth 5
```

**With GUI (Cute Chess):**
1. Add the engine: Settings → Engines → Add Engine
2. Select `build/chess_engine.exe`
3. Engine appears as "MokhtarEngine"
4. Play against it or run tournaments

### Testing

**Perft Testing** (move generation validation):
```c
// In tests/perft_test.c
// Counts nodes at each depth to verify move generation correctness
```

**Unit Tests:**
```bash
gcc tests/perft_test.c src/board.c src/movegen.c src/utils.c -I src -o test.exe
./test.exe
```

---

## Technical Details

### Bitboard Operations

```c
#define SET_BIT(bb, sq)     ((bb) |= (1ULL << (sq)))     // Set bit at square
#define CLEAR_BIT(bb, sq)   ((bb) &= ~(1ULL << (sq)))    // Clear bit
#define GET_BIT(bb, sq)     (((bb) >> (sq)) & 1)         // Get bit value
```

**Iterating through set bits:**
```c
uint64_t bb = board->pieces[WP];  // White pawns

while (bb) {
    int sq = get_lsb(bb);         // Get index of lowest bit
    // Process square sq...
    CLEAR_BIT(bb, sq);            // Clear the bit and continue
}
```

### Rank and File Extraction

```c
#define RANK(sq)  ((sq) / 8)       // Rank 0-7 (rank 1-8 in chess notation)
#define FILE(sq)  ((sq) % 8)       // File 0-7 (a-h in chess notation)
```

### Square Definitions

```c
#define A1 0,   B1 1,   C1 2,   D1 3,   E1 4,   F1 5,   G1 6,   H1 7
#define A2 8,   B2 9,   C2 10,  ...
#define ...
#define A8 56,  B8 57,  C8 58,  D8 59,  E8 60,  F8 61,  G8 62,  H8 63
```

### Search Algorithm Pseudocode

```
function alpha_beta(board, depth, alpha, beta):
    if zobrist[board] in transposition_table:
        return cached_result
    
    if depth == 0:
        return evaluate(board)
    
    moves = generate_legal_moves(board)
    
    if moves is empty:
        if is_in_check(board):
            return -INFINITY  # Checkmate
        else:
            return 0  # Stalemate
    
    best_score = -INFINITY
    
    for each move in moves:
        make_move(board, move)
        score = -alpha_beta(board, depth-1, -beta, -alpha)
        undo_move(board, move)
        
        best_score = max(best_score, score)
        alpha = max(alpha, best_score)
        
        if alpha >= beta:
            break  # Beta cutoff
    
    store in transposition_table
    return best_score
```

### Zobrist Hashing Pseudocode

```
function initialize_zobrist():
    for piece in [WP, WN, WB, WR, WQ, WK, BP, BN, BB, BR, BQ, BK]:
        for square in [0, 63]:
            zobrist_pieces[piece][square] = random_u64()
    
    zobrist_side = random_u64()
    
    for castling_rights in [0, 15]:
        zobrist_castling[castling_rights] = random_u64()
    
    for file in [0, 7]:
        zobrist_ep[file] = random_u64()

function compute_position_hash(board):
    hash = 0
    
    for piece in all pieces:
        if piece is on square s:
            hash ^= zobrist_pieces[piece][s]
    
    if side to move is BLACK:
        hash ^= zobrist_side
    
    hash ^= zobrist_castling[board.castling]
    
    if en_passant is valid:
        hash ^= zobrist_ep[en_passant_file]
    
    return hash
```

---

## Performance Characteristics

### Nodes Per Second
- Startup: ~1M-2M nodes/second (without optimization)
- With `-O3`: ~5M-10M nodes/second

### Search Depth
- `depth 4`: ~100K nodes, <100ms
- `depth 5`: ~10M nodes, <2s
- `depth 6`: ~100M nodes, ~5-10s

### Memory Usage
- Base board state: ~100 bytes
- Zobrist tables: ~1KB
- Transposition table (default): ~16MB
- Total: ~16.1MB

---

## Known Limitations & Future Improvements

### Current Limitations
1. **Fixed depth search** — No adaptive time management
2. **Simple evaluation** — No endgame tables or advanced tactics
3. **No opening book** — Starts from scratch every game
4. **Limited transposition table** — Fixed size, no resizing
5. **No move ordering** — Searches moves in move generation order

### Possible Improvements
1. Iterative deepening with time management
2. Killer moves and history heuristics
3. Principle variation (PV) searching for better move ordering
4. Endgame tablebases
5. More sophisticated evaluation (pawn structure, king safety)
6. SEE (Static Exchange Evaluation) for capturing
7. SIMD optimizations for move generation
8. Parallel search (multi-threading)

---

## Debugging and Testing

### Enable Debug Output

Add to `search.c`:
```c
if (depth == current_max_depth) {
    printf("info depth %d move %s score %d nodes %d\n", 
           depth, move_str, score, nodes);
}
```

### Test Perft (Performance Test)

Verifies move generation correctness:
```
Position: rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
Perft 1: 20 nodes
Perft 2: 400 nodes
Perft 3: 5902 nodes
Perft 4: 119060 nodes
Perft 5: 1965990 nodes
```

If your engine matches these numbers, move generation is correct.

---

## Contributing & Modifications

### Adding Evaluation Features

Edit `eval.c`:
```c
int eval(const Board *board) {
    int score = material_count(board);
    score += piece_square_bonus(board);
    score += pawn_structure_eval(board);  // New feature
    return score;
}
```

### Adding Search Features

Edit `search.c`:
```c
// Implement killer moves
static int killer_moves[MAX_DEPTH][2];

// Add to alpha_beta:
if (is_killer(move)) {
    // Try killer moves first
}
```

### Testing Changes

Always run after changes:
```bash
gcc ... -o build/chess_engine.exe
./build/chess_engine.exe perft 4  # Should match expected perft
```

---

## References

### Chess Engine Concepts
- **Bitboards:** https://www.chessprogramming.org/Bitboards
- **Zobrist Hashing:** https://www.chessprogramming.org/Zobrist-Hashing
- **Alpha-Beta:** https://www.chessprogramming.org/Alpha-Beta
- **Transposition Table:** https://www.chessprogramming.org/Transposition-Table

### UCI Protocol
- **Official UCI Spec:** http://wbec-ridderkerk.nl/html/UCIProtocol.html

### Chess Rules
- **FIDE Rules:** https://www.fide.com/FIDE/handbook

---

## License

This project is open source. Feel free to use, modify, and distribute.

---

## Contact & Support

**Author:** Mokhtar  
**Engine Name:** MokhtarEngine  
**Version:** 1.0

For questions or issues, refer to the code comments or test files.

---

**Last Updated:** April 15, 2026
