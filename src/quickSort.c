/* Quick sort: pick a pivot, partition around it, sort both sides.
 *
 * Choices made here, and why:
 *
 *  - Median-of-three pivot (first, middle, last). A plain "first element"
 *    pivot makes sorted and reversed inputs hit the O(n^2) worst case. The
 *    median of three turns those inputs into the best case instead.
 *
 *  - Hoare partition. Both scans stop on elements EQUAL to the pivot, so an
 *    input full of duplicates is still split near the middle. (Lomuto
 *    partition puts all equal keys on one side and degrades to O(n^2).)
 *
 *  - Recurse into the smaller part, loop on the larger part. This bounds the
 *    recursion depth by log2(n) even in a bad case, so the stack is O(log n).
 *
 * Quick sort is NOT stable: long-distance swaps can reorder equal keys.
 */
#include "sort.h"

#include <assert.h>
#include <stdlib.h>

#include "sortctx.h"

typedef struct QuickCtx {
    SortCtx base;
    char *pivot; /* copy of the pivot value; tmp is taken by sortSwap */
} QuickCtx;

/* Order a[lo], a[mid], a[hi] so that a[lo] <= a[mid] <= a[hi]. */
static void medianOfThree(SortCtx *c, size_t lo, size_t mid, size_t hi) {
    if (sortCompareAt(c, mid, lo) < 0) {
        sortSwap(c, mid, lo);
    }
    if (sortCompareAt(c, hi, mid) < 0) {
        sortSwap(c, hi, mid);
        if (sortCompareAt(c, mid, lo) < 0) {
            sortSwap(c, mid, lo);
        }
    }
}

/* Hoare partition of a[lo..hi] (inclusive, hi - lo >= 2).
 * Returns p such that every key in a[lo..p] <= pivot <= every key in a[p+1..hi],
 * and lo <= p < hi, so both parts are non-empty and strictly smaller. */
static size_t partition(QuickCtx *q, size_t lo, size_t hi) {
    SortCtx *c = &q->base;
    size_t mid = lo + (hi - lo) / 2;

    medianOfThree(c, lo, mid, hi);
    sortMove(c, q->pivot, sortElemAt(c, mid));

    /* a[lo] <= pivot <= a[hi] now act as sentinels: neither scan can run
     * past the ends of the range. */
    size_t i = lo;
    size_t j = hi;
    for (;;) {
        while (sortComparePtr(c, sortElemAt(c, i), q->pivot) < 0) {
            i++;
        }
        while (sortComparePtr(c, sortElemAt(c, j), q->pivot) > 0) {
            j--;
        }
        /* The sentinels must keep both scans inside [lo, hi]. */
        assert(i <= hi && lo <= j && j <= hi);
        if (i >= j) {
            return j;
        }
        sortSwap(c, i, j);
        i++;
        j--;
    }
}

static void quickRange(QuickCtx *q, size_t lo, size_t hi, size_t depth) {
    SortCtx *c = &q->base;

    sortNoteDepth(c, depth);
    while (lo < hi) {
        if (hi - lo == 1) { /* two elements: one compare is enough */
            if (sortCompareAt(c, hi, lo) < 0) {
                sortSwap(c, lo, hi);
            }
            return;
        }
        size_t p = partition(q, lo, hi);
        /* Progress guarantee: both parts are non-empty and strictly smaller.
         * If a bug breaks this, fail loudly instead of looping forever. */
        assert(lo <= p && p < hi);
        /* Recurse on the smaller side, keep looping on the larger side. */
        if (p - lo < hi - p) {
            quickRange(q, lo, p, depth + 1);
            lo = p + 1;
        } else {
            quickRange(q, p + 1, hi, depth + 1);
            hi = p;
        }
    }
}

void quickSort(void *base, size_t n, size_t size, SortCompare cmp, SortStats *stats) {
    QuickCtx q;
    if (!sortBegin(&q.base, base, n, size, cmp, stats)) {
        return;
    }
    q.pivot = (char *)malloc(size);
    if (q.pivot != NULL) {
        sortAddExtra(&q.base, size);
        quickRange(&q, 0, n - 1, 1);
        free(q.pivot);
    }
    sortEnd(&q.base);
}
