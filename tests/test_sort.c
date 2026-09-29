/* Unit tests: standard C only, no external framework.
 * Run: make test
 *
 * The tests also go through the common interface. They walk the
 * implementation table (SORT_ALGORITHMS) and run the same checks on every
 * sort, so adding a sort does not require touching this file.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bench.h"
#include "sort.h"

static int checks = 0;
static int failures = 0;

static void report(const char *algo, const char *name, int ok) {
    checks++;
    if (ok) {
        printf("ok    %-12s %s\n", algo, name);
        return;
    }
    failures++;
    printf("FAIL  %-12s %s\n", algo, name);
}

/* --- int arrays ---------------------------------------------------------- */

static void expectSorted(const SortAlgorithm *algo, const char *name,
                         const int input[], const int want[], size_t n) {
    int a[32];
    SortStats stats;

    memcpy(a, input, n * sizeof(int));
    algo->sort(a, n, sizeof(a[0]), sortCompareInt, &stats);

    int ok = (n == 0) || memcmp(a, want, n * sizeof(int)) == 0;
    report(algo->name, name, ok);
    if (!ok) {
        printf("      got :");
        for (size_t i = 0; i < n; i++) {
            printf(" %d", a[i]);
        }
        printf("\n      want:");
        for (size_t i = 0; i < n; i++) {
            printf(" %d", want[i]);
        }
        printf("\n");
    }
}

/* --- stability ----------------------------------------------------------- */

/* Uses Record/recordCompare from bench.h: sort by key, tag = input order.
 * Equal keys with increasing tags after sorting means the sort was stable. */
static void expectStable(const SortAlgorithm *algo) {
    enum { N = 60 };
    Record a[N];
    SortStats stats;

    /* keys 0..4 only: many duplicates are needed to expose instability */
    for (int i = 0; i < N; i++) {
        a[i].key = (i * 7) % 5;
        a[i].tag = i;
    }
    algo->sort(a, N, sizeof(a[0]), recordCompare, &stats);

    int ok = recordsSorted(a, N) && recordsStable(a, N);
    /* The measured result must match the table's `stable` claim. */
    report(algo->name, "stability matches the table's claim", ok == algo->stable);
}

/* --- random data vs qsort ------------------------------------------------ */

static void expectMatchesQsort(const SortAlgorithm *algo) {
    enum { N = 500 };
    int *a = malloc(N * sizeof(int));
    int *want = malloc(N * sizeof(int));
    SortStats stats;

    srand(20260901);
    for (int i = 0; i < N; i++) {
        a[i] = rand() % 100; /* narrow range so duplicates appear */
        want[i] = a[i];
    }
    qsort(want, N, sizeof(want[0]), sortCompareInt);
    algo->sort(a, N, sizeof(a[0]), sortCompareInt, &stats);

    report(algo->name, "500 random ints equal the qsort result",
           memcmp(a, want, N * sizeof(int)) == 0);
    free(a);
    free(want);
}

/* --- every size from 0 to 200 -------------------------------------------- */

/* Off-by-one bugs hide at small or odd sizes (partition bounds in quick sort,
 * empty subtrees in tree sort), so every n in 0..200 is tried. */
static void expectManySizes(const SortAlgorithm *algo) {
    enum { MAX_N = 200 };
    Record a[MAX_N];
    Record want[MAX_N];
    SortStats stats;
    int ok = 1;

    srand(20260902);
    for (size_t n = 0; n <= MAX_N && ok; n++) {
        for (size_t i = 0; i < n; i++) {
            a[i].key = rand() % 20;
            a[i].tag = (int)i;
            want[i] = a[i];
        }
        qsort(want, n, sizeof(want[0]), recordCompare);
        algo->sort(a, n, sizeof(a[0]), recordCompare, &stats);

        for (size_t i = 0; i < n; i++) {
            if (a[i].key != want[i].key) {
                ok = 0;
            }
        }
        if (algo->stable && !recordsStable(a, n)) {
            ok = 0;
        }
        if (!ok) {
            printf("      failed at n = %zu\n", n);
        }
    }
    report(algo->name, "n = 0..200 all sorted (and stable if claimed)", ok);
}

/* --- counters are filled ------------------------------------------------- */

static void expectStats(const SortAlgorithm *algo) {
    int a[] = {5, 1, 4, 2, 3};
    SortStats stats;

    algo->sort(a, 5, sizeof(a[0]), sortCompareInt, &stats);
    report(algo->name, "counters are filled",
           stats.compares > 0 && stats.moves > 0 &&
           stats.extraBytes >= sizeof(a[0]) && stats.maxDepth >= 1);
}

static void expectBenchRun(const SortAlgorithm *algo) {
    enum { N = 300 };
    Record input[N];

    makeInput(input, N, INPUT_FEW_UNIQUE, 20260903u);
    BenchResult r = benchRun(algo, input, N, 2);

    report(algo->name, "benchRun fills sorted/stable/counters",
           r.sorted && r.stable == algo->stable && r.millis >= 0.0 &&
           r.stats.compares > 0 && r.n == N && r.algo == algo);
}

static void runAll(const SortAlgorithm *algo) {
    {
        const int a[] = {6, 8, 5, 9, 10, 1, 7, 2, 4, 3};
        const int want[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        expectSorted(algo, "shuffled array", a, want, 10);
    }
    {
        const int a[] = {1, 2, 3, 4, 5};
        const int want[] = {1, 2, 3, 4, 5};
        expectSorted(algo, "already sorted", a, want, 5);
    }
    {
        const int a[] = {5, 4, 3, 2, 1};
        const int want[] = {1, 2, 3, 4, 5};
        expectSorted(algo, "reversed", a, want, 5);
    }
    {
        const int a[] = {3, 1, 3, 1, 2};
        const int want[] = {1, 1, 2, 3, 3};
        expectSorted(algo, "with duplicates", a, want, 5);
    }
    {
        const int a[] = {2, 2, 2, 2};
        const int want[] = {2, 2, 2, 2};
        expectSorted(algo, "all equal", a, want, 4);
    }
    {
        const int a[] = {42};
        const int want[] = {42};
        expectSorted(algo, "single element", a, want, 1);
    }
    {
        const int a[1] = {0};
        const int want[1] = {0};
        expectSorted(algo, "empty array", a, want, 0);
    }
    expectStable(algo);
    expectManySizes(algo);
    expectMatchesQsort(algo);
    expectStats(algo);
    expectBenchRun(algo);
    printf("\n");
}

/* --- algorithm-specific properties --------------------------------------- */

static void expectShapeProperties(void) {
    enum { N = 1024 };
    Record *a = malloc(N * sizeof(Record));
    SortStats stats;

    /* Plain BST on sorted input degenerates into a list: height == n. */
    makeInput(a, N, INPUT_SORTED, 1u);
    treeSort(a, N, sizeof(Record), recordCompare, &stats);
    report("treeSort", "sorted input builds a degenerate tree (height = n)",
           stats.maxDepth == N && stats.compares == (size_t)N * (N - 1) / 2);

    /* AVL keeps the height within 1.44 * log2(n + 2). */
    makeInput(a, N, INPUT_SORTED, 1u);
    treeSortBalanced(a, N, sizeof(Record), recordCompare, &stats);
    report("treeSortAVL", "sorted input keeps height <= 1.44 log2 n",
           recordsSorted(a, N) && stats.maxDepth <= (size_t)(1.44 * log2(N + 2.0)));

    /* Quick sort recurses only on the smaller part: depth <= log2(n) + 1. */
    makeInput(a, N, INPUT_REVERSED, 1u);
    quickSort(a, N, sizeof(Record), recordCompare, &stats);
    report("quickSort", "recursion depth <= log2(n) + 1",
           recordsSorted(a, N) && stats.maxDepth <= 11);

    /* The same bound must hold on random input, where splits are uneven. */
    makeInput(a, N, INPUT_RANDOM, 1u);
    quickSort(a, N, sizeof(Record), recordCompare, &stats);
    report("quickSort", "random input: recursion depth <= log2(n) + 1",
           recordsSorted(a, N) && stats.maxDepth <= 11);

    /* Bubble sort with early exit: sorted input costs one pass, no moves. */
    makeInput(a, N, INPUT_SORTED, 1u);
    bubbleSort(a, N, sizeof(Record), recordCompare, &stats);
    report("bubbleSort", "sorted input: n-1 compares, 0 moves",
           stats.compares == N - 1 && stats.moves == 0);
    free(a);
}

/* --- the measuring tool itself (bench.c) --------------------------------- */

static void expectInputShapes(void) {
    enum { N = 400 };
    Record a[N];

    makeInput(a, N, INPUT_SORTED, 1u);
    report("bench", "makeInput(sorted) is sorted", recordsSorted(a, N));

    makeInput(a, N, INPUT_REVERSED, 1u);
    report("bench", "makeInput(reversed) is descending", a[0].key > a[N - 1].key);

    makeInput(a, N, INPUT_NEARLY_SORTED, 1u);
    int out = 0;
    for (int i = 0; i < N; i++) {
        out += a[i].key != i;
    }
    report("bench", "makeInput(nearly-sorted) moves only a few keys", out > 0 && out <= N / 50);

    Record bad[4] = {{1, 1}, {1, 0}, {2, 2}, {2, 3}};
    report("bench", "detects equal keys whose order was flipped",
           recordsSorted(bad, 4) && !recordsStable(bad, 4));
}

int main(void) {
    for (size_t k = 0; k < SORT_ALGORITHM_COUNT; k++) {
        runAll(&SORT_ALGORITHMS[k]);
    }
    runAll(&TREE_SORT_BALANCED);
    expectShapeProperties();
    expectInputShapes();

    printf("\n%d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
