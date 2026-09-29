#include "bench.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

int recordCompare(const void *a, const void *b) {
    int x = ((const Record *)a)->key;
    int y = ((const Record *)b)->key;
    return (x > y) - (x < y);
}

const char *inputKindName(InputKind kind) {
    switch (kind) {
        case INPUT_RANDOM:        return "random";
        case INPUT_SORTED:        return "sorted";
        case INPUT_REVERSED:      return "reversed";
        case INPUT_NEARLY_SORTED: return "nearly-sorted";
        case INPUT_FEW_UNIQUE:    return "few-unique";
        default:                  return "unknown";
    }
}

/* rand() only guarantees 15 bits. Combine two calls so large n still gets
 * (almost) distinct random keys. */
static int randomKey(void) {
    return (int)((((unsigned)rand() << 15) ^ (unsigned)rand()) & 0x3fffffff);
}

void makeInput(Record *a, size_t n, InputKind kind, unsigned seed) {
    srand(seed); /* fixed seed: the same input on every run */
    for (size_t i = 0; i < n; i++) {
        switch (kind) {
            case INPUT_RANDOM:        a[i].key = randomKey(); break;
            case INPUT_SORTED:        a[i].key = (int)i; break;
            case INPUT_REVERSED:      a[i].key = (int)(n - i); break;
            case INPUT_NEARLY_SORTED: a[i].key = (int)i; break;
            case INPUT_FEW_UNIQUE:    a[i].key = rand() % 8; break;
            default:                  a[i].key = 0; break;
        }
    }
    if (kind == INPUT_NEARLY_SORTED && n > 1) {
        size_t swaps = n / 100 > 0 ? n / 100 : 1;
        for (size_t s = 0; s < swaps; s++) {
            size_t i = (size_t)randomKey() % n;
            size_t j = (size_t)randomKey() % n;
            int t = a[i].key;
            a[i].key = a[j].key;
            a[j].key = t;
        }
    }
    for (size_t i = 0; i < n; i++) {
        a[i].tag = (int)i; /* remember the input position */
    }
}

int recordsSorted(const Record *a, size_t n) {
    for (size_t i = 1; i < n; i++) {
        if (a[i - 1].key > a[i].key) {
            return 0;
        }
    }
    return 1;
}

int recordsStable(const Record *a, size_t n) {
    for (size_t i = 1; i < n; i++) {
        if (a[i - 1].key == a[i].key && a[i - 1].tag > a[i].tag) {
            return 0;
        }
    }
    return 1;
}

BenchResult benchRun(const SortAlgorithm *algo, const Record *input, size_t n, int reps) {
    BenchResult result;
    result.algo = algo;
    result.n = n;
    result.millis = 0.0;
    sortStatsReset(&result.stats);
    result.sorted = 0;
    result.stable = 0;

    if (reps < 1) {
        reps = 1;
    }
    Record *work = (Record *)malloc((n > 0 ? n : 1) * sizeof(Record));
    if (work == NULL) {
        return result;
    }

    clock_t spent = 0;
    for (int t = 0; t < reps; t++) {
        memcpy(work, input, n * sizeof(Record));
        /* Start the clock after copying: only the sort is timed. */
        clock_t begin = clock();
        algo->sort(work, n, sizeof(Record), recordCompare, &result.stats);
        spent += clock() - begin;
    }

    result.millis = (double)spent * 1000.0 / CLOCKS_PER_SEC / reps;
    result.sorted = recordsSorted(work, n);
    result.stable = recordsStable(work, n);
    free(work);
    return result;
}
