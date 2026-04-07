# Chess Rules & Engine Constraints

This document outlines the chess rules and constraints implemented in this engine.

---

## 1. Fifty Move Rule

When both players make **50 consecutive moves** without any pawn advancement or piece capture, the game is declared a draw.

- The engine tracks a `halfmove_clock` counter
- Resets to `0` on any pawn move or capture
- Increments by `1` on all other moves
- Draw is triggered when `halfmove_clock >= 100` (50 moves × 2 plies)

---

## 2. Threefold Repetition Rule

If the same board position occurs **three times** during a game (with the same side to move, castling rights, and en passant square), either player can claim a draw.

- The engine tracks this using **Zobrist hashing**
- Each position is hashed to a 64-bit key and stored in a history table
- A repeated hash appearing 3 times triggers the draw condition

---

## 3. En Passant

A special pawn capture that occurs when a pawn advances **two squares from its starting rank** and lands beside an opposing pawn.

- The opposing pawn may capture it as if it had only moved one square
- This right **expires after one move** if not used
- The engine stores the en passant target square in the board state (`en_passant_sq`)

---

## 4. Side to Move

White always moves first. Players alternate turns until the game ends.

- The engine stores a `side_to_move` flag in the board state
  - `0` = White
  - `1` = Black
- The flag flips after every move

---

## 5. Ply

A **ply** is a single move by one player.

- One full move = two plies (white ply + black ply)
- The engine uses ply count to track:
  - Search depth
  - Move history
  - Sequence of captures during the game
- Distinct from "move number" which counts pairs of moves

---

## 6. Castling Rights

Each side can castle **kingside (O-O)** or **queenside (O-O-O)** under the following conditions:

- Neither the king nor the relevant rook has previously moved
- No squares between the king and rook are occupied
- The king does not pass through or land on an attacked square
- The king is not currently in check

The engine stores four castling rights flags in the board state:

| Flag | Meaning |
|------|---------|
| `WK` | White kingside |
| `WQ` | White queenside |
| `BK` | Black kingside |
| `BQ` | Black queenside |

---

## 7. Stalemate

If the player to move has **no legal moves** but is **not in check**, the game is a draw.

- The engine detects this during move generation
- If `generate_moves()` returns an empty list and the king is not in check → stalemate

---

## 8. Insufficient Material

The game is automatically a draw if neither side has enough material to deliver checkmate.

Common cases:
- King vs King
- King + Bishop vs King
- King + Knight vs King
- King + Bishop vs King + Bishop (same colored squares)