/* Measures every sort with the same ruler.
 *
 * Algorithms (sort.c) and measurement (bench.c) are kept apart. A sort does
 * not know it is being measured, and the bench does not know which sort it
 * runs. The only link between them is SortAlgorithm.
 */
#ifndef BENCH_H
#define BENCH_H

#include <stddef.h>

#include "sort.h"

/* Element used for measurement. We sort by key; tag stores the input position.
 * If, after sorting, equal keys still have increasing tags, the sort was stable.
 * A plain int array cannot show stability, hence this 8-byte record. */
typedef struct Record {
    int key;
    int tag;
} Record;

/* Looks at key only. tag must not take part (otherwise every sort looks stable). */
int recordCompare(const void *a, const void *b);

/* Input shapes. Sorting performance depends heavily on the input. */
typedef enum InputKind {
    INPUT_RANDOM,        /* uniformly random keys */
    INPUT_SORTED,        /* already ascending */
    INPUT_REVERSED,      /* descending */
    INPUT_NEARLY_SORTED, /* ascending, then 1% of positions randomly swapped */
    INPUT_FEW_UNIQUE,    /* only 8 distinct keys: many duplicates */
    INPUT_KIND_COUNT
} InputKind;

const char *inputKindName(InputKind kind); /* short ASCII key, e.g. "random" */

/* Fill a[0..n-1] with the given shape. A fixed seed gives the same input every time. */
void makeInput(Record *a, size_t n, InputKind kind, unsigned seed);

int recordsSorted(const Record *a, size_t n); /* are the keys ascending? */
int recordsStable(const Record *a, size_t n); /* did equal keys keep their tag order? */

typedef struct BenchResult {
    const SortAlgorithm *algo;
    size_t n;
    double millis;   /* average time of one run */
    SortStats stats; /* counters of the last run */
    int sorted;      /* is the output sorted? checked before trusting anything */
    int stable;      /* was it actually stable? (measured, not the table's claim) */
} BenchResult;

/* Copy input, sort it `reps` times and keep the average time. Copy time is excluded. */
BenchResult benchRun(const SortAlgorithm *algo, const Record *input, size_t n, int reps);

#endif /* BENCH_H */
