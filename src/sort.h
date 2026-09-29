/* Sorting comparison assignment: three sorts behind one common interface.
 *
 * C has no interfaces or classes, so we use a struct of function pointers.
 * One SortAlgorithm value plays the role of one "implementation" of the
 * interface. The comparison contract is the same as the C library's qsort.
 *
 * The three algorithms compared:
 *   bubbleSort  - taught in class, O(n^2), stable, in place
 *   quickSort   - taught in class, O(n log n) average, unstable, in place
 *   treeSort    - NOT taught in class (binary search tree sort), O(n log n)
 *                 average, O(n^2) worst case, stable, O(n) extra memory
 */
#ifndef SORT_H
#define SORT_H

#include <stddef.h>

/* Comparison function, same contract as qsort:
 * negative if a < b, zero if a == b, positive if a > b.
 * Thanks to this function pointer, a sort never needs to know the element type. */
typedef int (*SortCompare)(const void *a, const void *b);

/* Counters collected during one sort. Wall-clock time is measured outside
 * (bench.c); here we keep only values that are reproducible on any machine. */
typedef struct SortStats {
    size_t compares;   /* number of calls to the comparison function */
    size_t moves;      /* number of element copies (one swap = 3 moves) */
    size_t extraBytes; /* peak heap memory allocated outside the input array */
    size_t maxDepth;   /* max recursion depth (quick sort) or tree height
                          (tree sort, = depth of the traversal stack).
                          Purely iterative code reports 1. */
} SortStats;

/* One sorting algorithm. This struct is the "interface" of the assignment. */
typedef struct SortAlgorithm {
    const char *name;
    const char *timeComplexity;  /* average time complexity (for the table) */
    const char *spaceComplexity; /* extra memory */
    int stable;                  /* claimed stability; the tests check it */
    int quadratic;               /* 1 if O(n^2) on average: skipped for huge n */
    /* Sort base[0..n-1] ascending, in place. Each element is `size` bytes.
     * If stats is NULL nothing is measured. */
    void (*sort)(void *base, size_t n, size_t size, SortCompare cmp, SortStats *stats);
} SortAlgorithm;

void bubbleSort(void *base, size_t n, size_t size, SortCompare cmp, SortStats *stats);
void quickSort(void *base, size_t n, size_t size, SortCompare cmp, SortStats *stats);
void treeSort(void *base, size_t n, size_t size, SortCompare cmp, SortStats *stats);

/* Extension of tree sort using a self-balancing (AVL) tree. It is not one of
 * the three compared sorts; it is used in one extra experiment that shows how
 * balancing removes tree sort's O(n^2) worst case. */
void treeSortBalanced(void *base, size_t n, size_t size, SortCompare cmp, SortStats *stats);

/* Table of the three compared implementations. Callers only walk this table. */
extern const SortAlgorithm SORT_ALGORITHMS[];
extern const size_t SORT_ALGORITHM_COUNT;

/* The balanced tree sort, described with the same struct. */
extern const SortAlgorithm TREE_SORT_BALANCED;

void sortStatsReset(SortStats *stats);
int sortCompareInt(const void *a, const void *b); /* default compare for int arrays */

#endif /* SORT_H */
