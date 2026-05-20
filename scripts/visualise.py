#!/usr/bin/env python3
"""
scripts/visualise.py
Plot Monte Carlo simulation output.

Usage:
    python visualise.py --paths results/paths_*.csv --summary results/summary_*.json
    python visualise.py --benchmark results/benchmark.csv
"""

import argparse
import csv
import json
import pathlib
import sys

try:
    import matplotlib.pyplot as plt
    import matplotlib.ticker as mticker
    import numpy as np
except ImportError:
    sys.exit("Install dependencies: pip install matplotlib numpy")


# ─────────────────────────────────────────────────────────────────
# Price fan chart
# ─────────────────────────────────────────────────────────────────

def plot_fan(paths_csv: str, summary_json: str | None, max_display: int = 300) -> None:
    paths = []
    with open(paths_csv) as f:
        for row in csv.reader(f):
            paths.append([float(x) for x in row])

    paths = np.array(paths)            # (P, T+1)
    T = paths.shape[1] - 1
    days = np.arange(T + 1)

    sample = paths[:max_display]

    fig, ax = plt.subplots(figsize=(12, 6))

    # Individual paths (light)
    for p in sample:
        ax.plot(days, p, color="#2563eb", alpha=0.04, linewidth=0.5)

    # Percentile bands
    p5  = np.percentile(paths, 5,  axis=0)
    p25 = np.percentile(paths, 25, axis=0)
    p50 = np.percentile(paths, 50, axis=0)
    p75 = np.percentile(paths, 75, axis=0)
    p95 = np.percentile(paths, 95, axis=0)

    ax.fill_between(days, p5,  p95, alpha=0.12, color="#2563eb", label="5th–95th pct")
    ax.fill_between(days, p25, p75, alpha=0.25, color="#2563eb", label="25th–75th pct")
    ax.plot(days, p50, color="#1d4ed8", linewidth=2, label="Median")
    ax.axhline(paths[0, 0], color="gray", linewidth=1, linestyle="--", label=f"S₀ = ${paths[0,0]:,.0f}")

    ax.set_title("Bitcoin Price — Monte Carlo Simulation (GBM)", fontsize=14, pad=12)
    ax.set_xlabel("Days")
    ax.set_ylabel("Price (USD)")
    ax.yaxis.set_major_formatter(mticker.StrMethodFormatter("${x:,.0f}"))
    ax.legend(loc="upper left", framealpha=0.9)
    ax.grid(True, linestyle="--", linewidth=0.4, alpha=0.5)

    out = pathlib.Path(paths_csv).parent / "fan_chart.png"
    fig.savefig(out, dpi=150, bbox_inches="tight")
    print(f"Saved fan chart: {out}")
    plt.show()


# ─────────────────────────────────────────────────────────────────
# Terminal price histogram
# ─────────────────────────────────────────────────────────────────

def plot_histogram(paths_csv: str) -> None:
    terminal = []
    with open(paths_csv) as f:
        for row in csv.reader(f):
            if row:
                terminal.append(float(row[-1]))

    terminal = np.array(terminal)
    S0       = None
    with open(paths_csv) as f:
        first = next(csv.reader(f))
        S0 = float(first[0]) if first else None

    fig, ax = plt.subplots(figsize=(10, 5))
    ax.hist(terminal, bins=80, color="#2563eb", alpha=0.75, edgecolor="none")
    if S0:
        ax.axvline(S0, color="red", linewidth=1.5, linestyle="--", label=f"S₀ = ${S0:,.0f}")
    ax.axvline(np.percentile(terminal, 5),  color="orange", linewidth=1.2, linestyle=":", label="5th pct")
    ax.axvline(np.percentile(terminal, 95), color="green",  linewidth=1.2, linestyle=":", label="95th pct")

    ax.set_title("Distribution of Terminal Bitcoin Prices", fontsize=14)
    ax.set_xlabel("Final Price (USD)")
    ax.set_ylabel("Frequency")
    ax.xaxis.set_major_formatter(mticker.StrMethodFormatter("${x:,.0f}"))
    ax.legend()
    ax.grid(True, linestyle="--", linewidth=0.4, alpha=0.5)

    out = pathlib.Path(paths_csv).parent / "histogram.png"
    fig.savefig(out, dpi=150, bbox_inches="tight")
    print(f"Saved histogram : {out}")
    plt.show()


# ─────────────────────────────────────────────────────────────────
# Benchmark chart
# ─────────────────────────────────────────────────────────────────

def plot_benchmark(benchmark_csv: str) -> None:
    rows = []
    with open(benchmark_csv) as f:
        reader = csv.DictReader(f)
        for row in reader:
            rows.append(row)

    if not rows:
        print("No benchmark data found.")
        return

    backends = sorted(set(r["backend"] for r in rows))
    paths_list = sorted(set(int(r["num_paths"]) for r in rows))
    colors = {"CUDA": "#f59e0b", "OpenMP": "#2563eb", "Serial": "#6b7280"}

    fig, ax = plt.subplots(figsize=(10, 5))
    for b in backends:
        xs = [int(r["num_paths"])  for r in rows if r["backend"] == b]
        ys = [float(r["elapsed_sec"]) for r in rows if r["backend"] == b]
        ax.plot(xs, ys, marker="o", label=b, color=colors.get(b, "black"))

    ax.set_title("Simulation Runtime by Backend", fontsize=14)
    ax.set_xlabel("Number of Paths")
    ax.set_ylabel("Elapsed Time (s)")
    ax.set_xscale("log")
    ax.legend()
    ax.grid(True, linestyle="--", linewidth=0.4, alpha=0.5)

    out = pathlib.Path(benchmark_csv).parent / "benchmark_chart.png"
    fig.savefig(out, dpi=150, bbox_inches="tight")
    print(f"Saved benchmark : {out}")
    plt.show()


# ─────────────────────────────────────────────────────────────────
# CLI
# ─────────────────────────────────────────────────────────────────

def main() -> None:
    parser = argparse.ArgumentParser(description="Visualise Monte Carlo results")
    parser.add_argument("--paths",     help="Path to paths CSV file")
    parser.add_argument("--summary",   help="Path to summary JSON file")
    parser.add_argument("--benchmark", help="Path to benchmark CSV file")
    args = parser.parse_args()

    if args.paths:
        plot_fan(args.paths, args.summary)
        plot_histogram(args.paths)
    if args.benchmark:
        plot_benchmark(args.benchmark)
    if not args.paths and not args.benchmark:
        parser.print_help()


if __name__ == "__main__":
    main()
