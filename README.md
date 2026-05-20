# Concurrent Order Book

A C++ limit order book benchmarked across five concurrency strategies to understand how synchronization choices affect throughput and latency at scale.

## What it does

Implements a price-time priority order book supporting limit, market, and cancel orders. Five designs are benchmarked against 1M synthetic orders per run:

| Strategy | Description |
|---|---|
| **Sequential** | Single thread, no synchronization |
| **Mutex** | 4 threads, one mutex per symbol |
| **Queue** | 4 producer threads feed a single shared `ConcurrentQueue`; one consumer thread |
| **Per-Symbol** | One thread per symbol, no sharing |
| **Per-Symbol Queue** | 4 producers route to per-symbol queues; one consumer per symbol |

Two additional experiments are run after the main benchmarks:

- **Scaling analysis** — per-symbol thread count swept 1→2→4→8, total order count scales proportionally
- **Workload mix sweep** — limit-heavy (80/10/10), default (60/20/20), and market-heavy (20/70/10) mixes compared at fixed thread count

## Results (sample)

```
Strategy          Peak Throughput
Sequential        ~7M orders/sec
Mutex             ~3.5M orders/sec  (contention hurts)
Queue             ~4.5M orders/sec
Per-Symbol        ~22M orders/sec   (3x over sequential)
Per-Symbol Queue  ~6M orders/sec    (queue overhead dominates)
```

Per-symbol parallelism wins because each thread owns its order book exclusively — no locks, no shared state. Mutex-based threading actually regresses below sequential because all four threads compete for the same four locks.

## Build

Requires a C++17 compiler and `make`.

```bash
make          # builds ./main and ./tests
make bench    # builds, runs benchmark, generates plots
make clean    # removes binaries, CSVs, and plots
```

Running tests:
```bash
./tests
```

## Reproducing the benchmark

```bash
./main        # runs all benchmarks, writes bench_*.csv
python3 plot.py   # reads CSVs, writes plots/*.png
```

Requires Python 3 with `pandas`, `matplotlib`, and `numpy`:
```bash
pip install pandas matplotlib numpy
```

Plots are saved to `plots/`:
- `throughput.png` — peak throughput per strategy
- `latency.png` — p50/p95/p99 latency per strategy
- `scaling.png` — sequential vs parallel throughput as thread count grows
- `workload.png` — throughput by order mix, sequential vs parallel

## Project structure

```
src/
  order_book.h/cpp       — price-time priority matching engine
  order_generator.h/cpp  — synthetic order stream generator
  concurrent_queue.h     — blocking MPSC queue (mutex + condition variable)
  main.cpp               — benchmark harness
  tests.cpp              — unit tests for order book correctness
plot.py                  — reads bench_*.csv, writes plots/*.png
Makefile
```
