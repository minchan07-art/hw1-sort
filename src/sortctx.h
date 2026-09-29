/* Working context shared by the sort implementations (implementation-only header).
 *
 * sort.h is the public interface; this header holds tools that only the
 * implementations use. Callers (main.c, bench.c, tests) do not include it.
 *
 * Names visible in a header are prefixed with sort- ; helpers used inside a
 * single file are hidden with static.
 */
#ifndef SORTCTX_H
#define SORTCTX_H

#include <stddef.h>

#include "sort.h"

/* Carries the five sort arguments as one bundle. tmp holds one element
 * temporarily (used by sortSwap). */
typedef struct SortCtx {
    char *base;
    size_t size;
    SortCompare cmp;
    SortStats *stats;
    char *tmp;
} SortCtx;

/* Common setup and teardown. If there is nothing to sort, sortBegin returns 0
 * and sortEnd must not be called. */
int sortBegin(SortCtx *c, void *base, size_t n, size_t size,
              SortCompare cmp, SortStats *stats);
void sortEnd(SortCtx *c);

char *sortElemAt(const SortCtx *c, size_t i);
int sortCompareAt(SortCtx *c, size_t i, size_t j);            /* a[i] vs a[j] */
int sortComparePtr(SortCtx *c, const void *x, const void *y); /* any two elements */
void sortMove(SortCtx *c, void *dst, const void *src);
void sortSwap(SortCtx *c, size_t i, size_t j);

/* Record extra heap memory currently in use; keeps the peak in stats. */
void sortAddExtra(SortCtx *c, size_t bytes);
void sortNoteDepth(SortCtx *c, size_t depth);

#endif /* SORTCTX_H */
