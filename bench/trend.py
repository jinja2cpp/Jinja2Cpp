#!/usr/bin/env python3
"""Keep the history of instruction and allocation counts and draw it.

The `trend` job of .github/workflows/benchmark.yml runs count.py on every push to master
and keeps the results on the `bench-data` branch:

  trend.py append --history history.jsonl --counts counts.json --sha SHA --date DATE --subject TEXT
      adds one record per commit (a commit already recorded is replaced);
  trend.py render --history history.jsonl --out DIR [--repo-url URL]
      writes DIR/README.md (a table of the latest counts against the previous and the first
      record) and DIR/charts/<benchmark>.svg (instructions and allocations per iteration over
      the recorded commits), which GitHub shows when you open the branch;
  trend.py drift --history history.jsonl [--window N] [--threshold F] [--out FILE]
      compares the latest record with the one N records earlier and reports the counts
      that grew by more than F. The PR gate compares a PR with its base only, so a few
      merges that each add 2% pass it; this catches their sum. Only cases that newly cross
      the threshold are reported, so one accepted regression is not reported on every merge.

Counts come from callgrind and the counting allocator, so a step in a chart is a change in
the code, not machine noise. The memory panel shows the bytes a loaded template keeps
(Load/*) and the peak heap use of one render (Render/*), from count.py's `memory`.
"""
import argparse
import html
import json
import pathlib
import re
import sys

# Chart tokens (light, dark): the reference palette of the data-viz guide.
STYLE = """
.surface { fill: #fcfcfb; } .line { stroke: #2a78d6; } .dot { fill: #2a78d6; stroke: #fcfcfb; }
.grid { stroke: #e1e0d9; } .axis { stroke: #c3c2b7; }
.title { fill: #0b0b0b; } .label { fill: #898781; }
@media (prefers-color-scheme: dark) {
  .surface { fill: #1a1a19; } .line { stroke: #3987e5; } .dot { fill: #3987e5; stroke: #1a1a19; }
  .grid { stroke: #2c2c2a; } .axis { stroke: #383835; }
  .title { fill: #ffffff; }
}
text { font-family: system-ui, -apple-system, "Segoe UI", sans-serif; font-size: 11px; }
.title { font-size: 12px; font-weight: 600; }
.label { font-variant-numeric: tabular-nums; }
"""

WIDTH, PANEL, TOP, LEFT, RIGHT, GAP = 420, 96, 30, 52, 12, 46


def load_history(path):
    if not path.exists():
        return []
    return [json.loads(line) for line in path.read_text().splitlines() if line.strip()]


def append(args):
    counts = json.loads(args.counts.read_text())
    record = {"sha": args.sha, "date": args.date, "subject": args.subject,
              "instructions": counts.get("instructions", {}), "allocations": counts.get("allocations", {}),
              "memory": counts.get("memory", {})}
    history = [r for r in load_history(args.history) if r["sha"] != args.sha]
    history.append(record)
    args.history.write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in history))
    return 0


def human(value):
    for limit, suffix in ((1e9, "G"), (1e6, "M"), (1e3, "K")):
        if value >= limit:
            return f"{value / limit:.3g}{suffix}"
    return f"{value:.3g}"


def nice_max(value):
    if value <= 0:
        return 1
    magnitude = 10 ** (len(str(int(value))) - 1)
    for step in (1, 2, 2.5, 5, 10):
        if step * magnitude >= value:
            return step * magnitude
    return 10 * magnitude


def panel(points, title, y0, unit):
    """One single-series line panel: points are (label, value or None)."""
    values = [v for _, v in points if v is not None]
    top = nice_max(max(values) * 1.05) if values else 1
    plot_w = WIDTH - LEFT - RIGHT
    n = len(points)

    def x(i):
        return LEFT + (plot_w / 2 if n == 1 else plot_w * i / (n - 1))

    def y(v):
        return y0 + PANEL - PANEL * v / top

    out = [f'<text class="title" x="{LEFT}" y="{y0 - 8}">{html.escape(title)}</text>']
    for frac in (0, 0.5, 1):
        gy = y0 + PANEL - PANEL * frac
        cls = "axis" if frac == 0 else "grid"
        out.append(f'<line class="{cls}" x1="{LEFT}" x2="{WIDTH - RIGHT}" y1="{gy:.1f}" y2="{gy:.1f}" stroke-width="1"/>')
        out.append(f'<text class="label" x="{LEFT - 6}" y="{gy + 4:.1f}" text-anchor="end">{human(top * frac)}</text>')
    segment = []
    segments = [segment]
    for i, (_, v) in enumerate(points):
        if v is None:
            segment = []
            segments.append(segment)
        else:
            segment.append(f"{x(i):.1f},{y(v):.1f}")
    for seg in segments:
        if len(seg) > 1:
            out.append(f'<polyline class="line" fill="none" stroke-width="2" stroke-linejoin="round" '
                       f'stroke-linecap="round" points="{" ".join(seg)}"/>')
    for i, (label, v) in enumerate(points):
        if v is None:
            continue
        last = i == n - 1
        r = 4 if last or n == 1 else 0
        # Every point keeps an invisible hover target with a tooltip; the latest is drawn
        out.append(f'<circle class="dot" cx="{x(i):.1f}" cy="{y(v):.1f}" r="{r}" stroke-width="2">'
                   f'<title>{html.escape(label)}: {v:,.0f} {unit}</title></circle>')
        out.append(f'<circle cx="{x(i):.1f}" cy="{y(v):.1f}" r="8" fill="transparent">'
                   f'<title>{html.escape(label)}: {v:,.0f} {unit}</title></circle>')
    return out


def memory_of(record, name):
    """The memory figure that matters for a benchmark: what a loaded template keeps
    (Load/*), the peak of a render (Render/*); None for records from before count.py had it."""
    if not record:
        return None
    key = "retained" if name.startswith("Load/") else "peak"
    return (record.get("memory", {}).get(name) or {}).get(key)


def memory_title(name):
    return "bytes kept by the loaded template" if name.startswith("Load/") else "peak bytes during a render"


def chart(name, history):
    labels = [f'{r["sha"][:7]} {r["date"][:10]} {r["subject"][:60]}' for r in history]
    instr = [(lbl, r["instructions"].get(name)) for lbl, r in zip(labels, history)]
    allocs = [(lbl, (r["allocations"].get(name) or {}).get("count")) for lbl, r in zip(labels, history)]
    memory = [(lbl, memory_of(r, name)) for lbl, r in zip(labels, history)]
    with_memory = any(v is not None for _, v in memory)
    height = TOP + PANEL + GAP + PANEL + 30 + (GAP + PANEL if with_memory else 0)
    body = panel(instr, f"{name}: instructions per iteration", TOP, "instructions")
    body += panel(allocs, "allocations per iteration", TOP + PANEL + GAP, "allocations")
    if with_memory:
        body += panel(memory, memory_title(name), TOP + 2 * (PANEL + GAP), "bytes")
    first, last = history[0], history[-1]
    axis_y = height - 10
    body.append(f'<text class="label" x="{LEFT}" y="{axis_y}">{first["sha"][:7]} {first["date"][:10]}</text>')
    if len(history) > 1:
        body.append(f'<text class="label" x="{WIDTH - RIGHT}" y="{axis_y}" text-anchor="end">'
                    f'{last["sha"][:7]} {last["date"][:10]}</text>')
    return (f'<svg xmlns="http://www.w3.org/2000/svg" width="{WIDTH}" height="{height}" '
            f'viewBox="0 0 {WIDTH} {height}" role="img" aria-label="{html.escape(name)} trend">'
            f'<style>{STYLE}</style><rect class="surface" width="100%" height="100%" rx="6"/>'
            + "".join(body) + "</svg>\n")


def change(current, base):
    if current is None or not base:
        return ""
    pct = (current / base - 1) * 100
    return "0%" if abs(pct) < 0.005 else f"{pct:+.2f}%"


def render(args):
    history = load_history(args.history)
    if not history:
        print("no history", file=sys.stderr)
        return 1
    charts = args.out / "charts"
    charts.mkdir(parents=True, exist_ok=True)
    names = sorted({n for r in history for n in r["instructions"]}, key=lambda n: (n.split("/", 1)[1], n))
    first, prev, last = history[0], history[-2] if len(history) > 1 else None, history[-1]
    lines = ["# Benchmark trend", "",
             f"Instruction and allocation counts per iteration on master, one record per commit "
             f"({len(history)} so far, latest [{last['sha'][:7]}]({args.repo_url}/commit/{last['sha']}) on "
             f"{last['date'][:10]}). Written by the `trend` job of `.github/workflows/benchmark.yml` "
             f"with `bench/trend.py`; the data is `history.jsonl`.", "",
             "Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one "
             "render for `Render/*`; vs first compares with the first record that has it.", "",
             "| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first "
             "| Memory | vs previous | vs first |",
             "|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|"]
    for name in names:
        cur = last["instructions"].get(name)
        alloc = (last["allocations"].get(name) or {}).get("count")
        mem = memory_of(last, name)
        first_mem = next((m for m in (memory_of(r, name) for r in history) if m is not None), None)

        def alloc_of(record):
            return (record["allocations"].get(name) or {}).get("count") if record else None

        lines.append(f"| [{name}](#{re.sub(r'[^a-z0-9_ -]', '', name.lower()).replace(' ', '-')}) | "
                     f"{'' if cur is None else f'{cur:,.0f}'} | "
                     f"{change(cur, prev['instructions'].get(name)) if prev else ''} | "
                     f"{change(cur, first['instructions'].get(name))} | "
                     f"{'' if alloc is None else f'{alloc:,}'} | "
                     f"{change(alloc, alloc_of(prev)) if prev else ''} | {change(alloc, alloc_of(first))} | "
                     f"{'' if mem is None else f'{mem:,}'} | "
                     f"{change(mem, memory_of(prev, name))} | {change(mem, first_mem)} |")
        file_name = re.sub(r"[^A-Za-z0-9_.-]+", "_", name) + ".svg"
        (charts / file_name).write_text(chart(name, history))
    lines += ["", "## Charts", ""]
    for name in names:
        file_name = re.sub(r"[^A-Za-z0-9_.-]+", "_", name) + ".svg"
        lines += [f"### {name}", "", f"![{name}](charts/{file_name})", ""]
    (args.out / "README.md").write_text("\n".join(lines))
    return 0


def drifted(history, end, window, threshold):
    """Counts of history[end] more than threshold above history[end - window], as
    {(name, metric): (base, current)}; a case missing from either record is skipped."""
    if end < 1:
        return {}
    base, cur = history[max(0, end - window)], history[end]
    found = {}
    for name, value in cur["instructions"].items():
        before = base["instructions"].get(name)
        if before and value > before * (1 + threshold):
            found[(name, "instructions")] = (before, value)
    for name, value in cur["allocations"].items():
        before = (base["allocations"].get(name) or {}).get("count")
        count = (value or {}).get("count")
        # A handful of allocations is too few for a percentage to mean anything
        if before and count is not None and count > max(before * (1 + threshold), before + 2):
            found[(name, "allocations")] = (before, count)
    return found


def drift(args):
    history = load_history(args.history)
    end = len(history) - 1
    now = drifted(history, end, args.window, args.threshold)
    before = drifted(history, end - 1, args.window, args.threshold)
    new = {k: v for k, v in now.items() if k not in before}
    if not new:
        print(f"No new drift above {args.threshold:.0%} over the last {args.window} records.")
        if args.out:
            args.out.write_text("")
        return 0
    base = history[max(0, end - args.window)]
    last = history[end]
    url = args.repo_url
    lines = [f"Counts on master grew by more than {args.threshold:.0%} between "
             f"[{base['sha'][:7]}]({url}/commit/{base['sha']}) and [{last['sha'][:7]}]({url}/commit/{last['sha']}) "
             f"({min(args.window, end)} recorded merges; latest: {last['subject']}).", "",
             "| Benchmark | Metric | Before | Now | Change |", "|---|---|---:|---:|---:|"]
    for (name, metric), (b, c) in sorted(new.items()):
        lines.append(f"| {name} | {metric} | {b:,.0f} | {c:,.0f} | {change(c, b)} |")
    commits = ", ".join(f"[{r['sha'][:7]}]({url}/commit/{r['sha']})" for r in history[max(0, end - args.window) + 1:end + 1])
    lines += ["", f"Commits in the window: {commits}.", "",
              "A benchmark case that was edited in the window also shows here; check "
              "`git log bench/cases` before bisecting. Charts: the `bench-data` branch."]
    text = "\n".join(lines) + "\n"
    print(text)
    if args.out:
        args.out.write_text(text)
    return 0


def main():
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    a = sub.add_parser("append")
    a.add_argument("--history", required=True, type=pathlib.Path)
    a.add_argument("--counts", required=True, type=pathlib.Path)
    a.add_argument("--sha", required=True)
    a.add_argument("--date", required=True)
    a.add_argument("--subject", default="")
    r = sub.add_parser("render")
    r.add_argument("--history", required=True, type=pathlib.Path)
    r.add_argument("--out", required=True, type=pathlib.Path)
    r.add_argument("--repo-url", default="https://github.com/jinja2cpp/Jinja2Cpp")
    d = sub.add_parser("drift")
    d.add_argument("--history", required=True, type=pathlib.Path)
    d.add_argument("--window", type=int, default=10)
    d.add_argument("--threshold", type=float, default=0.03)
    d.add_argument("--out", type=pathlib.Path)
    d.add_argument("--repo-url", default="https://github.com/jinja2cpp/Jinja2Cpp")
    args = ap.parse_args()
    return {"append": append, "render": render, "drift": drift}[args.cmd](args)


if __name__ == "__main__":
    sys.exit(main())
