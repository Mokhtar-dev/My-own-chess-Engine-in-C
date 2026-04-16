#include "tt.h"        /* own header — always first */
#include <string.h>    /* memset                    */


#define TT_SIZE 1000000

TTEntry tt[TT_SIZE];

void tt_init()
{
    memset(tt, 0, sizeof(tt));
}

void tt_store(uint64_t key, int depth, int flag, int score)
{
    int index = key % TT_SIZE;

    tt[index].key = key;
    tt[index].depth = depth;
    tt[index].flag = flag;
    tt[index].score = score;
}

int tt_probe(uint64_t key, int depth, int alpha, int beta, int* score)
{
    int index = key % TT_SIZE;

    if (tt[index].key == key)
    {
        if (tt[index].depth >= depth)
        {
            *score = tt[index].score;

            if (tt[index].flag == EXACT)
                return 1;

            if (tt[index].flag == ALPHA && *score <= alpha)
                return 1;

            if (tt[index].flag == BETA && *score >= beta)
                return 1;
        }
    }

    return 0;
}