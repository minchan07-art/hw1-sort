"""A tiny hand-written SVG chart tool, Python standard library only.

The repository uses no external libraries (no matplotlib). SVG is plain text,
so we emit just what we need. As a bonus, chart diffs are readable.

Coordinates follow SVG: (0, 0) is top-left and y grows downward.
"""

import math

# Light background so charts read on both GitHub light and dark themes.
BG = "#fbfbf9"
INK = "#24292f"      # text
MUTED = "#6e7781"    # ticks, helper lines
GRID = "#dfe1e4"

# Okabe-Ito colour-blind-safe palette; dash patterns differ too, so colour is never the only cue.
SERIES_STYLE = {
    "bubbleSort":  ("#E69F00", "7 4"),       # orange, dashed
    "quickSort":   ("#0072B2", "none"),      # blue, solid
    "treeSort":    ("#009E73", "2 3"),       # green, dotted
    "treeSortAVL": ("#8c4799", "4 2 1 2"),   # purple, dash-dot
    # series of the balanced-tree experiment: colour = tree kind, dash = input
    "treeSort (random)":    ("#009E73", "none"),
    "treeSort (sorted)":    ("#009E73", "7 4"),
    "treeSortAVL (random)": ("#8c4799", "none"),
    "treeSortAVL (sorted)": ("#8c4799", "7 4"),
}
EXTRA = [("#0072B2", "none"), ("#E69F00", "7 4"), ("#009E73", "2 3"),
         ("#8c4799", "4 2 1 2"), ("#CC79A7", "1 3")]
_assigned = {}

FONT = ("-apple-system, BlinkMacSystemFont, 'Segoe UI', Helvetica, Arial, "
        "'DejaVu Sans', sans-serif")


def style_for(name):
    """Algorithms get fixed colours; other series cycle through EXTRA."""
    if name in SERIES_STYLE:
        return SERIES_STYLE[name]
    if name not in _assigned:
        _assigned[name] = EXTRA[len(_assigned) % len(EXTRA)]
    return _assigned[name]


def esc(text):
    return (str(text).replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;"))


def si(value):
    """Readable numbers: 32M instead of 3.2e7."""
    for limit, suffix in ((1e9, "B"), (1e6, "M"), (1e3, "K")):
        if value >= limit:
            trimmed = value / limit
            text = f"{trimmed:.0f}" if trimmed >= 10 else f"{trimmed:.1f}"
            if "." in text:  # strip trailing zeros after the point only ("10" must stay "10")
                text = text.rstrip("0").rstrip(".")
            return text + suffix
    if value >= 1:
        return f"{value:.0f}"
    return f"{value:g}"


def ms(value):
    """Label for time (ms) bars: 0.004 and 27.9 must both read well on one axis."""
    if value >= 10:
        return f"{value:.0f}"
    if value >= 1:
        return f"{value:.1f}"
    return f"{value:.3f}".rstrip("0").rstrip(".")


def nice_ticks(hi, count=5):
    """Ticks from 0 to just above hi at human-friendly steps."""
    if hi <= 0:
        return [0], 1
    raw = hi / count
    mag = 10 ** math.floor(math.log10(raw))
    step = next(m * mag for m in (1, 2, 2.5, 5, 10) if raw <= m * mag)
    top = math.ceil(hi / step) * step
    return [step * i for i in range(int(round(top / step)) + 1)], top


class Canvas:
    """Collects SVG fragments and writes them to a file in one go."""

    def __init__(self, width, height, title, subtitle=None):
        self.w, self.h = width, height
        self.parts = []
        self.parts.append(
            f'<rect width="{width}" height="{height}" fill="{BG}"/>')
        self.text(20, 30, title, size=16, weight="600")
        if subtitle:
            self.text(20, 50, subtitle, size=11.5, fill=MUTED)

    def text(self, x, y, s, size=12, fill=INK, anchor="start", weight="400"):
        self.parts.append(
            f'<text x="{x:.1f}" y="{y:.1f}" font-size="{size}" fill="{fill}" '
            f'text-anchor="{anchor}" font-weight="{weight}">{esc(s)}</text>')

    def line(self, x1, y1, x2, y2, stroke=GRID, width=1, dash="none"):
        self.parts.append(
            f'<line x1="{x1:.1f}" y1="{y1:.1f}" x2="{x2:.1f}" y2="{y2:.1f}" '
            f'stroke="{stroke}" stroke-width="{width}" stroke-dasharray="{dash}"/>')

    def rect(self, x, y, w, h, fill):
        self.parts.append(
            f'<rect x="{x:.1f}" y="{y:.1f}" width="{w:.1f}" height="{h:.1f}" '
            f'fill="{fill}"/>')

    def polyline(self, points, stroke, dash="none", width=2.2):
        pts = " ".join(f"{x:.1f},{y:.1f}" for x, y in points)
        self.parts.append(
            f'<polyline points="{pts}" fill="none" stroke="{stroke}" '
            f'stroke-width="{width}" stroke-dasharray="{dash}" '
            f'stroke-linejoin="round" stroke-linecap="round"/>')

    def dot(self, x, y, fill, r=3.4):
        self.parts.append(
            f'<circle cx="{x:.1f}" cy="{y:.1f}" r="{r}" fill="{fill}"/>')

    def save(self, path):
        body = "\n  ".join(self.parts)
        svg = (f'<svg xmlns="http://www.w3.org/2000/svg" width="{self.w}" '
               f'height="{self.h}" viewBox="0 0 {self.w} {self.h}" '
               f'font-family="{FONT}">\n  {body}\n</svg>\n')
        with open(path, "w", encoding="utf-8") as f:
            f.write(svg)
        return path


def _legend(canvas, names, x, y):
    for name in names:
        color, dash = style_for(name)
        canvas.line(x, y - 4, x + 22, y - 4, stroke=color, width=2.4, dash=dash)
        canvas.dot(x + 11, y - 4, color, r=3)
        canvas.text(x + 28, y, name, size=11.5)
        x += 28 + len(name) * 6.6 + 18


def line_chart(path, title, subtitle, xs, series, xlabel, ylabel,
               log_axes=True, annotate_slope=None, vline=None, vline_label=""):
    """Line chart. With log_axes both axes are logarithmic.

    Linear axes show the SIZE of the gap; log-log axes show the GROWTH RATE
    (the slope is the complexity exponent).

    xs      : x values (e.g. [1000, 2000, 4000, 8000])
    series  : {name: [y values, ...]}
    """
    W, H = 720, 420
    L, R, T, B = 78, 24, 88, 58
    c = Canvas(W, H, title, subtitle)
    if annotate_slope is None:
        annotate_slope = log_axes  # slope only means something on log-log

    ys_all = [v for values in series.values() for v in values if v > 0]
    lo, hi = min(ys_all), max(ys_all)

    if log_axes:
        y0, y1 = math.floor(math.log10(lo)), math.ceil(math.log10(hi))
        x0, x1 = math.log10(min(xs)), math.log10(max(xs))
        yticks = [10 ** e for e in range(y0, y1 + 1)]

        def px(v):
            return L + (math.log10(v) - x0) / (x1 - x0) * (W - L - R)

        def py(v):
            return H - B - (math.log10(v) - y0) / (y1 - y0) * (H - T - B)
    else:
        yticks, top = nice_ticks(hi)
        x0, x1 = min(xs), max(xs)

        def px(v):
            return L + (v - x0) / (x1 - x0) * (W - L - R)

        def py(v):
            return H - B - v / top * (H - T - B)

    for t in yticks:
        y = py(t)
        c.line(L, y, W - R, y)
        c.text(L - 10, y + 4, si(t), size=10.5, fill=MUTED, anchor="end")
    # x axis: ticks only at the measured n values
    last_label = None
    for v in xs:
        x = px(v)
        c.line(x, T, x, H - B, dash="2 4")
        if last_label is None or x - last_label > 60:  # skip crowded labels
            c.text(x, H - B + 18, f"{v:,}", size=10.5, fill=MUTED, anchor="middle")
            last_label = x

    if vline is not None:
        x = px(vline)
        c.line(x, T, x, H - B, stroke=INK, width=1.4, dash="5 4")
        c.text(x, T - 4, vline_label, size=11, fill=INK, anchor="middle", weight="600")

    c.line(L, H - B, W - R, H - B, stroke=MUTED)
    c.line(L, T, L, H - B, stroke=MUTED)
    c.text((L + W - R) / 2, H - 14, xlabel, size=11.5, fill=MUTED, anchor="middle")
    c.text(20, T - 14, ylabel, size=11.5, fill=MUTED)

    placed = []
    for name, values in series.items():
        color, dash = style_for(name)
        points = [(px(x), py(v)) for x, v in zip(xs, values) if v > 0]
        c.polyline(points, color, dash)
        for x, y in points:
            c.dot(x, y, color)
        if annotate_slope and len(values) > 1 and values[0] > 0:
            slope = ((math.log10(values[-1]) - math.log10(values[0]))
                     / (math.log10(xs[-1]) - math.log10(xs[0])))
            ex, ey = points[-1]
            ey -= 11
            # keep slope labels of neighbouring lines from overlapping
            while any(abs(ey - used) < 14 for used in placed):
                ey -= 14
            placed.append(ey)
            c.text(ex - 6, ey, f"slope {slope:.2f}", size=11,
                   fill=color, anchor="end", weight="600")

    _legend(c, list(series), L, H - 30)
    return c.save(path)


def grouped_bar_chart(path, title, subtitle, groups, series, ylabel,
                      log_scale=False, value_label=si):
    """Grouped bars. groups = x-axis group names, series = {name: [value per group]}."""
    W, H = 720, 420
    L, R, T, B = 78, 24, 88, 62
    c = Canvas(W, H, title, subtitle)

    values = [v for vs in series.values() for v in vs]
    hi = max(values)
    positive = [v for v in values if v > 0]
    lo = min(positive) if positive else 1

    if log_scale:
        y0 = math.floor(math.log10(lo))
        y1 = math.ceil(math.log10(hi))

        def py(v):
            v = max(v, 10 ** y0)
            return H - B - (math.log10(v) - y0) / (y1 - y0) * (H - T - B)

        ticks = [10 ** e for e in range(y0, y1 + 1)]
    else:
        ticks, y1 = nice_ticks(hi)

        def py(v):
            return H - B - v / y1 * (H - T - B)

    for t in ticks:
        y = py(t)
        c.line(L, y, W - R, y)
        c.text(L - 10, y + 4, si(t), size=10.5, fill=MUTED, anchor="end")

    c.line(L, H - B, W - R, H - B, stroke=MUTED)
    c.text(20, T - 14, ylabel, size=11.5, fill=MUTED)

    span = (W - L - R) / len(groups)
    names = list(series)
    bar_w = min(38, span / (len(names) + 1.4))
    for gi, group in enumerate(groups):
        center = L + span * (gi + 0.5)
        start = center - bar_w * len(names) / 2
        for si_, name in enumerate(names):
            color, _ = style_for(name)
            v = series[name][gi]
            top = py(v) if v > 0 else H - B
            x = start + bar_w * si_
            c.rect(x, top, bar_w - 3, max(H - B - top, 0.8), color)
            c.text(x + (bar_w - 3) / 2, top - 5, value_label(v), size=9.5,
                   fill=MUTED, anchor="middle")
        c.text(center, H - B + 18, group, size=11.5, anchor="middle")

    _legend(c, names, L, H - 26)
    return c.save(path)
