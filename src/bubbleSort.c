/* Bubble sort: compare neighbours and push the larger one to the right.
 *
 * Each pass "bubbles" the largest remaining element to the end of the
 * unsorted part. With the early-exit flag, an already sorted input finishes
 * after one pass (n - 1 compares, 0 moves). Every swap costs 3 moves, which
 * is why bubble sort is slow even compared to other O(n^2) sorts.
 */
#include "sort.h"

#include "sortctx.h"

void bubbleSort(void *base, size_t n, size_t size, SortCompare cmp, SortStats *stats) {
    SortCtx c;
    if (!sortBegin(&c, base, n, size, cmp, stats)) {
        return;
    }
    /* After each pass the largest value of a[0..end-1] is at a[end-1]. */
    for (size_t end = n; end > 1; end--) {
        int swapped = 0;
        for (size_t j = 1; j < end; j++) {
            /* Strict '>' so equal elements are never swapped (stable). */
            if (sortCompareAt(&c, j - 1, j) > 0) {
                sortSwap(&c, j - 1, j);
                swapped = 1;
            }
        }
        /* No swap during a whole pass means the array is already sorted. */
        if (!swapped) {
            break;
        }
    }
    sortEnd(&c);
}
