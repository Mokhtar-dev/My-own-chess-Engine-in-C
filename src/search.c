#include "search.h"    /* own header — always first */
#include "board.h"     /* Board, make_move, undo    */
#include "movegen.h"   /* MoveList, generate_moves  */
#include "eval.h"      /* eval                      */
#include "tt.h"        /* tt_probe, tt_store        */

int nodes = 0;

/* ================================
   Alpha-Beta Search
================================ */
int alpha_beta(Board* board, int depth, int alpha, int beta)
{
    int original_alpha = alpha;

    // TT probe
    int tt_score;
    if (tt_probe(board->zobrist_key, depth, alpha, beta, &tt_score))
        return tt_score;

    // leaf node
    if (depth == 0)
        return eval(board);

    nodes++;

    MoveList list;
    list.count = 0;
    generate_moves(board, &list);

    // no moves → mate or stalemate
    if (list.count == 0)
        return eval(board);

    int best_score = -INF;

    for (int i = 0; i < list.count; i++)
    {
        int move = list.moves[i];

        UndoInfo undo;
        if (!make_move(board, move, &undo))
            continue; // illegal move

        int score = -alpha_beta(board, depth - 1, -beta, -alpha);

        undo_move(board, move, &undo);

        if (score > best_score)
            best_score = score;

        if (score > alpha)
            alpha = score;

        // beta cutoff
        if (alpha >= beta)
            break;
    }

    // ================= TT STORE =================
    int flag = EXACT;

    if (best_score <= original_alpha)
        flag = ALPHA;
    else if (best_score >= beta)
        flag = BETA;

    tt_store(board->zobrist_key, depth, flag, best_score);

    return best_score;
}

/* ================================
   Root search
================================ */
int search_position(Board* board, int depth, int* best_move)
{
    MoveList list;
    list.count = 0;
    generate_moves(board, &list);

    int best_score = -INF;
    *best_move = 0;

    for (int i = 0; i < list.count; i++)
    {
        int move = list.moves[i];

        UndoInfo undo;
        if (!make_move(board, move, &undo))
            continue;

        int score = -alpha_beta(board, depth - 1, -INF, INF);

        undo_move(board, move, &undo);

        if (score > best_score)
        {
            best_score = score;
            *best_move = move;
        }
    }

    return best_score;
}