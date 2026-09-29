# Sorting Algorithm Comparison: Bubble, Quick and Tree Sort

Advanced Algorithms (SIT2001-01), Fall 2026, Assignment 1.

Three sorting algorithms behind one common C interface, measured with the same
inputs, counters and timer:

| Algorithm | Taught in class | Average time | Extra memory | Stable |
|---|---|---|---|---|
| `bubbleSort` | yes | O(n^2) | O(1) | yes |
| `quickSort` | yes | O(n log n) | O(log n) | no |
| `treeSort` | **no** (binary search tree sort) | O(n log n), worst O(n^2) | O(n) | yes |

An AVL-balanced tree sort (`treeSortBalanced`) is included for one extra experiment.

- Report: submitted separately as a PDF
- Raw measurements behind the report: [report/results.csv](report/results.csv),
  [report/tree-balance.csv](report/tree-balance.csv), with charts in [report/](report/)

Built from the course template [lec-algorithm/algorithm-env](https://github.com/lec-algorithm/algorithm-env).

## Run

Open in GitHub Codespaces, or locally:

```sh
docker compose up -d
docker compose exec lab bash
```

Inside the container:

| Command | What it does |
|---|---|
| `make test` | 57 unit checks (must end with `0 failures`) |
| `make run` | Human-readable comparison table |
| `make charts` | Re-measure; rewrite `report/*.csv` and `report/*.svg` (times will differ slightly, counts stay the same) |
| `python3 tools/plot.py --reuse` | Redraw the charts from the saved CSV without measuring again |
| `./src/main.out --csv` | All measurements as CSV |
| `./src/main.out --tree` | Plain BST vs AVL tree sort experiment (CSV) |
| `make clean` | Remove build outputs |

## Layout

```plaintext
src/
  sort.h, sort.c         common interface (SortAlgorithm) and implementation table
  sortctx.h              helpers shared by the sorts (counted compare/move/swap)
  bubbleSort.c           bubble sort with early exit
  quickSort.c            median-of-three pivot, Hoare partition, O(log n) stack
  treeSort.c             BST tree sort (+ AVL variant), index-based nodes, iterative
  bench.h, bench.c       input shapes, timing, sortedness and stability checks
  main.c                 runs the measurements (table / CSV)
tests/test_sort.c        unit tests, standard C only
tools/plot.py            charts as SVG, Python standard library only
tools/svgchart.py
report/                  charts (SVG) and the raw measurements (CSV)
```

## Conventions

- Standard libraries only (C17, Python 3); no external dependencies.
- Executables are named `*.out` (ignored by git).
- Builds without warnings under `-Wall -Wextra`.
