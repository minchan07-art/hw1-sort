/* Sorting comparison: bubble / quick / tree.
 *
 *   make run                a human-readable comparison table
 *   ./src/main.out --csv    the same measurements as CSV (used by tools/plot.py)
 *   ./src/main.out --tree   extra experiment: plain BST vs AVL tree sort (CSV)
 *
 * The caller never names a sort. It walks the implementation table
 * (SORT_ALGORITHMS). What to measure is written in exactly one place: SPECS.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bench.h"
#include "sort.h"

#define SEED 20260901u

/* --- what to measure ----------------------------------------------------- */

typedef struct Spec {
    const char *scope; /* kinds: per input shape / growth: n grows / scale: large n */
    InputKind kind;
    size_t n;
    int reps;
} Spec;

static const Spec SPECS[] = {
    {"kinds", INPUT_RANDOM, 4000, 3},
    {"kinds", INPUT_SORTED, 4000, 3},
    {"kinds", INPUT_REVERSED, 4000, 3},
    {"kinds", INPUT_NEARLY_SORTED, 4000, 3},
    {"kinds", INPUT_FEW_UNIQUE, 4000, 3},
    {"growth", INPUT_RANDOM, 1000, 3},
    {"growth", INPUT_RANDOM, 2000, 3},
    {"growth", INPUT_RANDOM, 4000, 3},
    {"growth", INPUT_RANDOM, 8000, 1},
    {"growth", INPUT_RANDOM, 16000, 1},
    /* O(n^2) sorts are skipped here: bubble sort would need minutes. */
    {"scale", INPUT_RANDOM, 16000, 5},
    {"scale", INPUT_RANDOM, 64000, 3},
    {"scale", INPUT_RANDOM, 256000, 3},
    {"scale", INPUT_RANDOM, 1024000, 3},
};

static const size_t SPEC_COUNT = sizeof(SPECS) / sizeof(SPECS[0]);

static int skipped(const Spec *spec, const SortAlgorithm *algo) {
    return strcmp(spec->scope, "scale") == 0 && algo->quadratic;
}

/* Receives one measured row. Only the output format differs between the
 * table and the CSV: the same trick as swapping sorts via function pointers. */
typedef void (*RowSink)(const Spec *spec, const BenchResult *r);

static void measureAll(RowSink sink, void (*onSpec)(const Spec *spec)) {
    for (size_t s = 0; s < SPEC_COUNT; s++) {
        const Spec *spec = &SPECS[s];
        Record *input = (Record *)malloc(spec->n * sizeof(Record));
        if (input == NULL) {
            return;
        }
        makeInput(input, spec->n, spec->kind, SEED);
        if (onSpec != NULL) {
            onSpec(spec);
        }
        for (size_t k = 0; k < SORT_ALGORITHM_COUNT; k++) {
            if (skipped(spec, &SORT_ALGORITHMS[k])) {
                continue;
            }
            BenchResult r = benchRun(&SORT_ALGORITHMS[k], input, spec->n, spec->reps);
            sink(spec, &r);
        }
        free(input);
    }
}

/* --- human-readable table ------------------------------------------------ */

#define ROW_FORMAT "%-12s %10.3f %12zu %11zu %11zu B %7zu %6s %6s\n"
#define ROW_HEADER "algorithm       time(ms)     compares       moves    extra mem   depth  sorted stable\n"
#define ROW_RULE   "-----------------------------------------------------------------------------------------\n"

static void tableRow(const Spec *spec, const BenchResult *r) {
    (void)spec;
    printf(ROW_FORMAT, r->algo->name, r->millis, r->stats.compares, r->stats.moves,
           r->stats.extraBytes, r->stats.maxDepth, r->sorted ? "yes" : "NO!",
           r->stable ? "yes" : "no");
}

static void tableSpecHeader(const Spec *spec) {
    static const char *lastScope = NULL;

    if (lastScope == NULL || strcmp(lastScope, spec->scope) != 0) {
        if (strcmp(spec->scope, "kinds") == 0) {
            printf("By input shape (n = %zu, average of %d runs)\n", spec->n, spec->reps);
        } else if (strcmp(spec->scope, "growth") == 0) {
            printf("\nGrowing n (random input)\n");
        } else {
            printf("\nLarge n (random input, O(n^2) sorts skipped)\n");
        }
        lastScope = spec->scope;
    }
    if (strcmp(spec->scope, "kinds") == 0) {
        printf("\n[%s]\n", inputKindName(spec->kind));
    } else {
        printf("\n[n = %zu]\n", spec->n);
    }
    printf("%s%s", ROW_HEADER, ROW_RULE);
}

/* Show what the table claims first, so it can be compared with the measurements. */
static void printDeclarations(void) {
    printf("Implementation table (what SortAlgorithm claims)\n");
    printf("algorithm    time           extra mem  stability\n");
    printf("%s", ROW_RULE);
    for (size_t k = 0; k < SORT_ALGORITHM_COUNT; k++) {
        const SortAlgorithm *algo = &SORT_ALGORITHMS[k];
        printf("%-12s %-14s %-10s %s\n", algo->name, algo->timeComplexity,
               algo->spaceComplexity, algo->stable ? "stable" : "unstable");
    }
    printf("\n");
}

static void reportTable(void) {
    printf("=== Sorting comparison: bubble / quick / tree ===\n");
    printf("Elements are (key, tag) records of %zu bytes. Sorted by key; tag checks stability.\n\n",
           sizeof(Record));
    printDeclarations();
    measureAll(tableRow, tableSpecHeader);

    printf("\nHow to read\n");
    printf("  time      : only comparable on the same machine; compares/moves are exact.\n");
    printf("  extra mem : peak heap memory outside the input array.\n");
    printf("  depth     : quick sort = max recursion depth, tree sort = tree height.\n");
    printf("  stable    : measured from tag order, not the table's claim.\n");
}

/* --- machine-readable CSV ------------------------------------------------ */

static void csvRow(const Spec *spec, const BenchResult *r) {
    printf("%s,%s,%zu,%s,%.4f,%zu,%zu,%zu,%zu,%d,%d\n", spec->scope,
           inputKindName(spec->kind), spec->n, r->algo->name, r->millis,
           r->stats.compares, r->stats.moves, r->stats.extraBytes,
           r->stats.maxDepth, r->sorted, r->stable);
}

static void reportCsv(void) {
    printf("scope,input,n,algo,millis,compares,moves,extraBytes,maxDepth,sorted,stable\n");
    measureAll(csvRow, NULL);
}

/* --- extra experiment: does balancing fix tree sort? --------------------- */

static void reportTreeExperiment(void) {
    static const size_t SIZES[] = {1000, 2000, 4000, 8000, 16000};
    static const InputKind KINDS[] = {INPUT_RANDOM, INPUT_SORTED};
    const SortAlgorithm *algos[] = {&SORT_ALGORITHMS[2], &TREE_SORT_BALANCED};

    printf("input,n,algo,millis,compares,moves,extraBytes,maxDepth,sorted,stable\n");
    for (size_t k = 0; k < sizeof(KINDS) / sizeof(KINDS[0]); k++) {
        for (size_t s = 0; s < sizeof(SIZES) / sizeof(SIZES[0]); s++) {
            size_t n = SIZES[s];
            Record *input = (Record *)malloc(n * sizeof(Record));
            if (input == NULL) {
                return;
            }
            makeInput(input, n, KINDS[k], SEED);
            for (size_t a = 0; a < 2; a++) {
                BenchResult r = benchRun(algos[a], input, n, 3);
                printf("%s,%zu,%s,%.4f,%zu,%zu,%zu,%zu,%d,%d\n", inputKindName(KINDS[k]),
                       n, r.algo->name, r.millis, r.stats.compares, r.stats.moves,
                       r.stats.extraBytes, r.stats.maxDepth, r.sorted, r.stable);
            }
            free(input);
        }
    }
}

int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "--csv") == 0) {
        reportCsv();
        return 0;
    }
    if (argc > 1 && strcmp(argv[1], "--tree") == 0) {
        reportTreeExperiment();
        return 0;
    }
    reportTable();
    return 0;
}
