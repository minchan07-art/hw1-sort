"""Draw the comparison charts (SVG) from the benchmark CSV.

    make charts                  # measure again, then draw
    python3 tools/plot.py --reuse   # redraw from the saved CSV only

Runs `src/main.out --csv` and `--tree`, saves the raw numbers to
report/results.csv and report/tree-balance.csv, and writes SVG charts to
report/. The charts and the report tables therefore come from the same run.

Python standard library only; tools/svgchart.py draws the SVG.
"""

import csv
import io
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import svgchart  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
BINARY = ROOT / "src" / "main.out"
OUT_DIR = ROOT / "report"
ALGOS = ["bubbleSort", "quickSort", "treeSort"]
KINDS = ["random", "sorted", "reversed", "nearly-sorted", "few-unique"]
INT_FIELDS = ("n", "compares", "moves", "extraBytes", "maxDepth")


def run_csv(flag):
    """Run the benchmark binary (building it if needed) and parse its CSV."""
    subprocess.run(["make", "-s", "src/main.out"], cwd=ROOT, check=True)
    result = subprocess.run([str(BINARY), flag], cwd=ROOT, check=True,
                            capture_output=True, text=True)
    rows = list(csv.DictReader(io.StringIO(result.stdout)))
    for row in rows:
        for key in INT_FIELDS:
            row[key] = int(row[key])
        row["millis"] = float(row["millis"])
    return rows


def save(rows, name):
    with open(OUT_DIR / name, "w", encoding="utf-8", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)


def pick(rows, **conditions):
    return [r for r in rows if all(r[k] == v for k, v in conditions.items())]


def by_algo(rows, field, keys, key_field, algos=ALGOS):
    """Collect {algorithm: [value for each key, in order]}."""
    table = {}
    for algo in algos:
        values = []
        for key in keys:
            match = [r for r in rows if r["algo"] == algo and r[key_field] == key]
            values.append(match[0][field] if match else 0)
        table[algo] = values
    return table


def load_saved(name):
    """Read a CSV saved by an earlier run (used with --reuse)."""
    with open(OUT_DIR / name, encoding="utf-8") as f:
        rows = list(csv.DictReader(f))
    for row in rows:
        for key in INT_FIELDS:
            row[key] = int(row[key])
        row["millis"] = float(row["millis"])
    return rows


def main():
    OUT_DIR.mkdir(exist_ok=True)
    if "--reuse" in sys.argv:
        # Redraw from the saved numbers without measuring again.
        rows = load_saved("results.csv")
        tree = load_saved("tree-balance.csv")
    else:
        rows = run_csv("--csv")
        save(rows, "results.csv")
        tree = run_csv("--tree")
        save(tree, "tree-balance.csv")

    kinds = pick(rows, scope="kinds")
    growth = pick(rows, scope="growth")
    scale = pick(rows, scope="scale")
    sizes = sorted({r["n"] for r in growth})
    big = sorted({r["n"] for r in scale})
    made = []

    # Each chart is drawn twice, as in the course sample: linear axes show the
    # SIZE of the gap, log axes show small values and the GROWTH RATE.
    #
    # 1. Growth with n
    for field, unit, noun, stem in (
            ("compares", "compares", "Comparisons", "growth-compares"),
            ("millis", "time (ms)", "Running time", "growth-time")):
        data = by_algo(growth, field, sizes, "n")
        made.append(svgchart.line_chart(
            OUT_DIR / f"{stem}.svg",
            f"{noun} as n grows (linear axes)",
            "Random input. The size of the gap is visible, small values sit on the floor",
            sizes, data, "n (elements)", unit, log_axes=False))
        made.append(svgchart.line_chart(
            OUT_DIR / f"{stem}-log.svg",
            f"{noun} as n grows (log-log axes)",
            "Slope = complexity exponent (2.0 means n^2, about 1.1 means n log n)",
            sizes, data, "n (elements)", unit))

    # 2. By input shape, n = 4,000
    for field, unit, stem, label, fmt in (
            ("millis", "time (ms)", "input-shapes-time", "Running time", svgchart.ms),
            ("compares", "compares", "input-shapes-compares", "Comparisons", svgchart.si),
            ("moves", "moves", "input-shapes-moves", "Element moves", svgchart.si)):
        data = by_algo(kinds, field, KINDS, "input")
        made.append(svgchart.grouped_bar_chart(
            OUT_DIR / f"{stem}.svg",
            f"{label} by input shape (linear axis)",
            "n = 4,000. Bar height is proportional to the value; small values vanish",
            KINDS, data, unit, value_label=fmt))
        made.append(svgchart.grouped_bar_chart(
            OUT_DIR / f"{stem}-log.svg",
            f"{label} by input shape (log axis)",
            "Same data. Small values become readable on the log axis",
            KINDS, data, unit, log_scale=True, value_label=fmt))
    made.append(svgchart.grouped_bar_chart(
        OUT_DIR / "input-shapes-depth.svg",
        "Recursion depth / tree height by input shape (log axis)",
        "n = 4,000. Bubble is always 1, quick stays near log2 n, the tree follows the input",
        KINDS, by_algo(kinds, "maxDepth", KINDS, "input"), "depth / height",
        log_scale=True))

    # Compares vs moves on reversed input: why bubble sort is so heavy
    rev = pick(kinds, input="reversed")
    made.append(svgchart.grouped_bar_chart(
        OUT_DIR / "compares-vs-moves.svg",
        "Comparisons vs moves on reversed input (log axis)",
        "n = 4,000. Every bubble comparison leads to a swap, and one swap is 3 moves",
        ["compares", "moves"],
        {a: [pick(rev, algo=a)[0]["compares"], pick(rev, algo=a)[0]["moves"]] for a in ALGOS},
        "count", log_scale=True))

    # 3. Large n: quick vs tree (linear axes show the real gap)
    made.append(svgchart.line_chart(
        OUT_DIR / "scale-time.svg",
        "Quick sort vs tree sort up to n = 1,024,000 (linear axes)",
        "Random input. Same number of comparisons, but the tree loses on memory access",
        big, by_algo(scale, "millis", big, "n", ALGOS[1:]), "n (elements)",
        "time (ms)", log_axes=False))
    made.append(svgchart.line_chart(
        OUT_DIR / "scale-memory.svg",
        "Extra heap memory as n grows (log-log)",
        "Quick sort: two element slots. Tree sort: 2 indices + 1 output slot per element",
        big, by_algo(scale, "extraBytes", big, "n", ALGOS[1:]), "n (elements)",
        "bytes", annotate_slope=True))

    # 4. Extra experiment: plain BST vs AVL on sorted input
    tsizes = sorted({r["n"] for r in tree})
    series = {}
    for inp in ("random", "sorted"):
        for algo in ("treeSort", "treeSortAVL"):
            series[f"{algo} ({inp})"] = [
                pick(tree, input=inp, algo=algo, n=n)[0]["compares"] for n in tsizes]
    made.append(svgchart.line_chart(
        OUT_DIR / "tree-balance.svg",
        "Tree sort: plain BST vs self-balancing AVL (comparisons, log-log)",
        "Sorted input turns the plain BST into a list (slope 2). AVL stays n log n",
        tsizes, series, "n (elements)", "compares"))

    for path in made:
        print(f"wrote {Path(path).relative_to(ROOT)}")


if __name__ == "__main__":
    main()
