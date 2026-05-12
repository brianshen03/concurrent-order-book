# Project Proposal: Synchronization Strategies in a High-Throughput Order Book

## Introduction
This project studies how different synchronization strategies affect the performance of a high-throughput limit order book. I will implement multiple concurrent designs in C++ and benchmark their scalability under controlled workloads. The goal is to understand how contention impacts throughput and latency in a shared-state system.

## Problem
Order matching systems must process large volumes of events while preserving strict ordering constraints. This creates a tension between correctness and parallelism.

This project asks: How do different synchronization strategies scale under increasing concurrency and contention?

## Approach
We build a limit order book supporting:
- Limit, market, and cancel orders
- Price-time priority matching

We compare three designs:
1. Single-threaded baseline
2. Mutex-based implementation
3. Queue-based / parallel ingestion design

Synthetic order streams are generated with controllable:
- Order mix
- Arrival rate
- Book depth

## Evaluation
We benchmark across:
- Thread count
- Workload type
- Contention level

Metrics:
- Throughput (orders/sec)
- Latency (p50 / p95 / p99)
- Scaling efficiency

## Expected Outcome
We expect to identify:
- When locking becomes a bottleneck
- How different designs scale with load
- Tradeoffs between simplicity, correctness, and performance