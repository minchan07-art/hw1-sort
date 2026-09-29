/* Common base: helpers shared by the implementations, and the implementation table.
 *
 * Each algorithm lives in its own file:
 *   bubbleSort.c . quickSort.c . treeSort.c
 */
#include "sort.h"

#include <stdlib.h>
#include <string.h>

#include "sortctx.h"

void sortStatsReset(SortStats *stats) {
    if (stats == NULL) {
        return;
    }
    stats->compares = 0;
    stats->moves = 0;
    stats->extraBytes = 0;
    stats->maxDepth = 1; /* iterative code still counts as depth 1 */
}

int sortCompareInt(const void *a, const void *b) {
    int x = *(const int *)a;
    int y = *(const int *)b;
    return (x > y) - (x < y); /* subtraction could overflow, so we avoid it */
}

/* --- working context --------------------------------------------------- */

int sortBegin(SortCtx *c, void *base, size_t n, size_t size,
              SortCompare cmp, SortStats *stats) {
    sortStatsReset(stats);
    if (base == NULL || cmp == NULL || size == 0 || n < 2) {
        return 0;
    }
    c->base = (char *)base;
    c->size = size;
    c->cmp = cmp;
    c->stats = stats;
    c->tmp = (char *)malloc(size); /* one element slot for swaps */
    if (c->tmp == NULL) {
        return 0;
    }
    if (stats != NULL) {
        stats->extraBytes = size;
    }
    return 1;
}

void sortEnd(SortCtx *c) {
    free(c->tmp);
    c->tmp = NULL;
}

char *sortElemAt(const SortCtx *c, size_t i) {
    return c->base + i * c->size;
}

int sortCompareAt(SortCtx *c, size_t i, size_t j) {
    return sortComparePtr(c, sortElemAt(c, i), sortElemAt(c, j));
}

int sortComparePtr(SortCtx *c, const void *x, const void *y) {
    if (c->stats != NULL) {
        c->stats->compares++;
    }
    return c->cmp(x, y);
}

void sortMove(SortCtx *c, void *dst, const void *src) {
    memcpy(dst, src, c->size);
    if (c->stats != NULL) {
        c->stats->moves++;
    }
}

void sortSwap(SortCtx *c, size_t i, size_t j) {
    sortMove(c, c->tmp, sortElemAt(c, i));
    sortMove(c, sortElemAt(c, i), sortElemAt(c, j));
    sortMove(c, sortElemAt(c, j), c->tmp);
}

void sortAddExtra(SortCtx *c, size_t bytes) {
    if (c->stats != NULL) {
        c->stats->extraBytes += bytes;
    }
}

void sortNoteDepth(SortCtx *c, size_t depth) {
    if (c->stats != NULL && depth > c->stats->maxDepth) {
        c->stats->maxDepth = depth;
    }
}

/* --- implementation table ---------------------------------------------- */

/* Adding a sort = one more file plus one more line here.
 * main.c and the tests only walk this table. */
const SortAlgorithm SORT_ALGORITHMS[] = {
    {"bubbleSort", "O(n^2)",     "O(1)",     1, 1, bubbleSort},
    {"quickSort",  "O(n log n)", "O(log n)", 0, 0, quickSort},
    {"treeSort",   "O(n log n)", "O(n)",     1, 0, treeSort},
};

const size_t SORT_ALGORITHM_COUNT = sizeof(SORT_ALGORITHMS) / sizeof(SORT_ALGORITHMS[0]);

const SortAlgorithm TREE_SORT_BALANCED =
    {"treeSortAVL", "O(n log n)", "O(n)", 1, 0, treeSortBalanced};
