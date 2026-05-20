#!/usr/bin/env python3
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
import numpy as np
import os

os.makedirs("plots", exist_ok=True)

STRATEGY_ORDER  = ["sequential", "mutex", "queue", "per_symbol", "per_symbol_queue"]
STRATEGY_LABELS = ["Sequential", "Mutex", "Queue", "Per-Symbol", "Per-Symbol\nQueue"]
COLORS = ["#4C72B0", "#DD8452", "#55A868", "#C44E52", "#8172B2"]

# ── 1. Throughput comparison ──────────────────────────────────────────────────

runs = pd.read_csv("bench_runs.csv")
best_tput = (
    runs.groupby("strategy")["throughput"].max() / 1e6
)  # M orders/sec

fig, ax = plt.subplots(figsize=(8, 5))
values = [best_tput[s] for s in STRATEGY_ORDER]
bars = ax.bar(STRATEGY_LABELS, values, color=COLORS, width=0.55, edgecolor="white", linewidth=0.8)

for bar, val in zip(bars, values):
    ax.text(bar.get_x() + bar.get_width() / 2, bar.get_height() + 0.3,
            f"{val:.1f}M", ha="center", va="bottom", fontsize=9, fontweight="bold")

ax.set_ylabel("Throughput (M orders/sec)")
ax.set_title("Peak Throughput by Concurrency Strategy")
ax.yaxis.set_major_formatter(mticker.FormatStrFormatter("%.0fM"))
ax.set_ylim(0, max(values) * 1.15)
ax.spines["top"].set_visible(False)
ax.spines["right"].set_visible(False)
fig.tight_layout()
fig.savefig("plots/throughput.png", dpi=150)
plt.close(fig)
print("Saved plots/throughput.png")

# ── 2. Latency percentiles ────────────────────────────────────────────────────

# For each strategy take the run with the highest throughput
best_run_idx = runs.groupby("strategy")["throughput"].idxmax()
best_rows = runs.loc[best_run_idx].set_index("strategy")

percentile_cols = ["p50_ns", "p95_ns", "p99_ns"]
pct_labels = ["p50", "p95", "p99"]
x = np.arange(len(STRATEGY_ORDER))
width = 0.25

fig, ax = plt.subplots(figsize=(9, 5))
for i, (col, label) in enumerate(zip(percentile_cols, pct_labels)):
    vals = [best_rows.loc[s, col] for s in STRATEGY_ORDER]
    offset = (i - 1) * width
    bars = ax.bar(x + offset, vals, width, label=label,
                  color=COLORS[i + 1], edgecolor="white", linewidth=0.8)

ax.set_xticks(x)
ax.set_xticklabels(STRATEGY_LABELS)
ax.set_ylabel("Latency (ns)")
ax.set_title("Order Submission Latency by Strategy (best run)")
ax.legend(title="Percentile")
ax.yaxis.set_major_formatter(mticker.FuncFormatter(lambda v, _: f"{int(v)}ns"))
ax.spines["top"].set_visible(False)
ax.spines["right"].set_visible(False)
fig.tight_layout()
fig.savefig("plots/latency.png", dpi=150)
plt.close(fig)
print("Saved plots/latency.png")

# ── 3. Scaling analysis ───────────────────────────────────────────────────────

scaling = pd.read_csv("bench_scaling.csv")
scaling["seq_tput_m"] = scaling["seq_tput"] / 1e6
scaling["par_tput_m"] = scaling["par_tput"] / 1e6
scaling["ideal_tput_m"] = scaling["seq_tput_m"].iloc[0] * scaling["n_symbols"]

fig, ax = plt.subplots(figsize=(7, 5))
ax.plot(scaling["n_symbols"], scaling["seq_tput_m"], "o-", color=COLORS[0],
        label="Sequential", linewidth=2, markersize=6)
ax.plot(scaling["n_symbols"], scaling["par_tput_m"], "s-", color=COLORS[2],
        label="Parallel (per-symbol)", linewidth=2, markersize=6)
ax.plot(scaling["n_symbols"], scaling["ideal_tput_m"], "--", color="#999999",
        label="Ideal linear", linewidth=1.5)

ax.set_xlabel("Symbol count (= thread count)")
ax.set_ylabel("Throughput (M orders/sec)")
ax.set_title("Scaling Analysis: Per-Symbol Parallel vs Sequential")
ax.set_xticks(scaling["n_symbols"])
ax.yaxis.set_major_formatter(mticker.FormatStrFormatter("%.0fM"))
ax.legend()
ax.spines["top"].set_visible(False)
ax.spines["right"].set_visible(False)
fig.tight_layout()
fig.savefig("plots/scaling.png", dpi=150)
plt.close(fig)
print("Saved plots/scaling.png")

# ── 4. Workload mix sweep ─────────────────────────────────────────────────────

workload = pd.read_csv("bench_workload.csv")
workload["mix"] = workload["mix"].str.strip()
workload["seq_tput_m"] = workload["seq_tput"] / 1e6
workload["par_tput_m"] = workload["par_tput"] / 1e6

x = np.arange(len(workload))
width = 0.35

fig, ax = plt.subplots(figsize=(8, 5))
b1 = ax.bar(x - width / 2, workload["seq_tput_m"], width, label="Sequential",
            color=COLORS[0], edgecolor="white", linewidth=0.8)
b2 = ax.bar(x + width / 2, workload["par_tput_m"], width, label="Parallel (per-symbol)",
            color=COLORS[2], edgecolor="white", linewidth=0.8)

for bar, val in zip(list(b1) + list(b2),
                    list(workload["seq_tput_m"]) + list(workload["par_tput_m"])):
    ax.text(bar.get_x() + bar.get_width() / 2, bar.get_height() + 0.2,
            f"{val:.1f}M", ha="center", va="bottom", fontsize=8)

ax.set_xticks(x)
ax.set_xticklabels(workload["mix"], fontsize=9)
ax.set_ylabel("Throughput (M orders/sec)")
ax.set_title("Workload Mix Sweep: Sequential vs Parallel")
ax.yaxis.set_major_formatter(mticker.FormatStrFormatter("%.0fM"))
ax.set_ylim(0, workload["par_tput_m"].max() * 1.15)
ax.legend()
ax.spines["top"].set_visible(False)
ax.spines["right"].set_visible(False)
fig.tight_layout()
fig.savefig("plots/workload.png", dpi=150)
plt.close(fig)
print("Saved plots/workload.png")
