#include <iostream>
#include <iomanip>
#include <cstdio>
#include <fstream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <mutex>
#include <unordered_map>
#include "order_book.h"
#include "order_generator.h"
#include "concurrent_queue.h"

const int NUM_THREADS = 4;
using Clock = std::chrono::high_resolution_clock;
using SymOrders = std::unordered_map<std::string, std::vector<Order>>;

// ─── helpers ────────────────────────────────────────────────────────────────

void print_latency_stats(std::vector<long>& latencies) {
    std::sort(latencies.begin(), latencies.end());
    size_t n = latencies.size();
    std::cout << " p50=" << latencies[n * 0.50] << "ns"
              << " p95=" << latencies[n * 0.95] << "ns"
              << " p99=" << latencies[n * 0.99] << "ns";
}

void print_run(int r, size_t n_orders, int trades, long us) {
    std::cout << "Run " << r + 1 << ": "
              << "Orders=" << n_orders << " Trades=" << trades
              << " Time=" << us << " us"
              << " Throughput=" << (n_orders * 1000000.0 / us) << " orders/sec";
}

// latencies must already be sorted (print_latency_stats does this in-place)
void write_run_csv(std::ofstream& csv, const std::string& strategy, int r,
                   size_t n_orders, long us, const std::vector<long>& latencies) {
    size_t n = latencies.size();
    csv << strategy << "," << r + 1 << ","
        << (long long)(n_orders * 1000000.0 / us) << ","
        << latencies[n * 0.50] << ","
        << latencies[n * 0.95] << ","
        << latencies[n * 0.99] << "\n";
}

// ─── thread workers ─────────────────────────────────────────────────────────

void process_range(std::unordered_map<std::string, OrderBook>& books,
                   std::unordered_map<std::string, std::mutex>& mutexes,
                   const std::vector<Order>& orders, int start, int end,
                   std::vector<int>& trade_counts, int thread_idx,
                   std::vector<long>& latencies) {
    for (int i = start; i < end; i++) {
        const std::string& sym = orders[i].symbol;
        std::lock_guard<std::mutex> lock(mutexes.at(sym));
        auto t0 = Clock::now();
        auto trades = books.at(sym).submit(orders[i]);
        auto t1 = Clock::now();
        latencies.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
        trade_counts[thread_idx] += trades.size();
    }
}

void producer(ConcurrentQueue& q, const std::vector<Order>& orders, int start, int end) {
    for (int i = start; i < end; i++) q.push(orders[i]);
}

void consumer(std::unordered_map<std::string, OrderBook>& books, ConcurrentQueue& q,
              int& trade_count, std::vector<long>& latencies) {
    Order o;
    while (q.pop(o)) {
        auto t0 = Clock::now();
        auto trades = books.at(o.symbol).submit(o);
        auto t1 = Clock::now();
        latencies.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
        trade_count += trades.size();
    }
}

void symbol_producer(std::unordered_map<std::string, ConcurrentQueue*>& queues,
                     const std::vector<Order>& orders, int start, int end) {
    for (int i = start; i < end; i++) queues[orders[i].symbol]->push(orders[i]);
}

void symbol_consumer(OrderBook& book, ConcurrentQueue& q,
                     std::atomic<int>& trade_count, std::vector<long>& latencies) {
    Order o;
    while (q.pop(o)) {
        auto t0 = Clock::now();
        auto trades = book.submit(o);
        auto t1 = Clock::now();
        latencies.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
        trade_count += trades.size();
    }
}

// ─── benchmark runs ─────────────────────────────────────────────────────────

void run_sequential(const std::vector<std::string>& symbols,
                    const std::vector<Order>& orders,
                    const SymOrders& symbol_orders,
                    int runs, std::ofstream& csv) {
    std::cout << "==========SEQUENTIAL RUNS==========\n";
    for (int r = 0; r < runs; r++) {
        std::unordered_map<std::string, OrderBook> books;
        for (const auto& s : symbols) books.try_emplace(s, 950, 1050);
        int total_trades = 0;
        std::vector<long> latencies;
        latencies.reserve(orders.size());

        auto start = Clock::now();
        for (const auto& s : symbols)
            for (const auto& o : symbol_orders.at(s)) {
                auto t0 = Clock::now();
                auto trades = books.at(s).submit(o);
                auto t1 = Clock::now();
                latencies.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
                total_trades += trades.size();
            }
        auto end = Clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

        print_run(r, orders.size(), total_trades, us);
        print_latency_stats(latencies);
        std::cout << "\n";
        write_run_csv(csv, "sequential", r, orders.size(), us, latencies);
    }
}

void run_mutex(const std::vector<std::string>& symbols,
               const std::vector<Order>& orders,
               int runs, std::ofstream& csv) {
    std::cout << "==========MUTEX RUNS==========\n";
    for (int r = 0; r < runs; r++) {
        std::unordered_map<std::string, OrderBook> books;
        std::unordered_map<std::string, std::mutex> mutexes;
        for (const auto& s : symbols) { books.try_emplace(s, 950, 1050); mutexes[s]; }

        int size = orders.size();
        int chunk = (size + NUM_THREADS - 1) / NUM_THREADS;
        std::vector<std::thread> threads;
        std::vector<int> trade_counts(NUM_THREADS, 0);
        std::vector<std::vector<long>> per_thread_latencies(NUM_THREADS);

        auto start = Clock::now();
        for (int i = 0; i < NUM_THREADS; i++) {
            int chunk_start = i * chunk;
            int chunk_end = std::min(chunk_start + chunk, size);
            if (chunk_start < chunk_end) {
                per_thread_latencies[i].reserve(chunk_end - chunk_start);
                threads.emplace_back(process_range, std::ref(books), std::ref(mutexes),
                                     std::cref(orders), chunk_start, chunk_end,
                                     std::ref(trade_counts), i,
                                     std::ref(per_thread_latencies[i]));
            }
        }
        for (auto& th : threads) th.join();
        auto end = Clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

        int total_trades = 0;
        for (int count : trade_counts) total_trades += count;

        std::vector<long> latencies;
        latencies.reserve(orders.size());
        for (auto& v : per_thread_latencies)
            latencies.insert(latencies.end(), v.begin(), v.end());

        print_run(r, orders.size(), total_trades, us);
        print_latency_stats(latencies);
        std::cout << "\n";
        write_run_csv(csv, "mutex", r, orders.size(), us, latencies);
    }
}

void run_queue(const std::vector<std::string>& symbols,
               const std::vector<Order>& orders,
               int runs, std::ofstream& csv) {
    std::cout << "==========QUEUE RUNS==========\n";
    for (int r = 0; r < runs; r++) {
        ConcurrentQueue cq;
        std::unordered_map<std::string, OrderBook> books;
        for (const auto& s : symbols) books.try_emplace(s, 950, 1050);
        int trade_count = 0;
        std::vector<long> latencies;
        latencies.reserve(orders.size());

        int size = orders.size();
        int chunk = (size + NUM_THREADS - 1) / NUM_THREADS;
        std::vector<std::thread> producers;

        auto start = Clock::now();
        for (int i = 0; i < NUM_THREADS; i++) {
            int chunk_start = i * chunk;
            int chunk_end = std::min(chunk_start + chunk, size);
            if (chunk_start < chunk_end)
                producers.emplace_back(producer, std::ref(cq), std::cref(orders), chunk_start, chunk_end);
        }
        std::thread consumer_thread(consumer, std::ref(books), std::ref(cq),
                                    std::ref(trade_count), std::ref(latencies));
        for (auto& t : producers) t.join();
        cq.shutdown();
        consumer_thread.join();
        auto end = Clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

        print_run(r, orders.size(), trade_count, us);
        print_latency_stats(latencies);
        std::cout << "\n";
        write_run_csv(csv, "queue", r, orders.size(), us, latencies);
    }
}

void run_per_symbol(const std::vector<std::string>& symbols,
                    const std::vector<Order>& orders,
                    const SymOrders& symbol_orders,
                    int runs, std::ofstream& csv) {
    std::cout << "==========PER-SYMBOL RUNS==========\n";
    for (int r = 0; r < runs; r++) {
        std::unordered_map<std::string, OrderBook> books;
        for (const auto& s : symbols) books.try_emplace(s, 950, 1050);

        std::atomic<int> total_trades{0};
        std::vector<std::thread> threads;
        std::vector<std::vector<long>> per_thread_latencies(symbols.size());

        auto start = Clock::now();
        for (int i = 0; i < (int)symbols.size(); i++) {
            const auto& s = symbols[i];
            per_thread_latencies[i].reserve(symbol_orders.at(s).size());
            threads.emplace_back([&books, &symbol_orders, &total_trades,
                                   &per_thread_latencies, s, i]() {
                int local_trades = 0;
                for (const auto& o : symbol_orders.at(s)) {
                    auto t0 = Clock::now();
                    auto trades = books.at(s).submit(o);
                    auto t1 = Clock::now();
                    per_thread_latencies[i].push_back(
                        std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
                    local_trades += trades.size();
                }
                total_trades += local_trades;
            });
        }
        for (auto& t : threads) t.join();
        auto end = Clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

        std::vector<long> latencies;
        latencies.reserve(orders.size());
        for (auto& v : per_thread_latencies)
            latencies.insert(latencies.end(), v.begin(), v.end());

        print_run(r, orders.size(), total_trades.load(), us);
        print_latency_stats(latencies);
        std::cout << "\n";
        write_run_csv(csv, "per_symbol", r, orders.size(), us, latencies);
    }
}

void run_per_symbol_queue(const std::vector<std::string>& symbols,
                          const std::vector<Order>& orders,
                          const SymOrders& symbol_orders,
                          int runs, std::ofstream& csv) {
    std::cout << "==========PER-SYMBOL QUEUE RUNS==========\n";
    for (int r = 0; r < runs; r++) {
        std::unordered_map<std::string, ConcurrentQueue> queues;
        std::unordered_map<std::string, OrderBook> books;
        for (const auto& s : symbols) { queues[s]; books.try_emplace(s, 950, 1050); }

        std::unordered_map<std::string, ConcurrentQueue*> queue_ptrs;
        for (const auto& s : symbols) queue_ptrs[s] = &queues[s];

        std::atomic<int> total_trades{0};
        std::vector<std::vector<long>> per_consumer_latencies(symbols.size());

        int size = orders.size();
        int chunk = (size + NUM_THREADS - 1) / NUM_THREADS;
        std::vector<std::thread> producers;
        std::vector<std::thread> consumers;

        auto start = Clock::now();
        for (int i = 0; i < NUM_THREADS; i++) {
            int chunk_start = i * chunk;
            int chunk_end = std::min(chunk_start + chunk, size);
            if (chunk_start < chunk_end)
                producers.emplace_back(symbol_producer, std::ref(queue_ptrs),
                                       std::cref(orders), chunk_start, chunk_end);
        }
        for (int i = 0; i < (int)symbols.size(); i++) {
            const auto& s = symbols[i];
            per_consumer_latencies[i].reserve(symbol_orders.at(s).size());
            consumers.emplace_back(symbol_consumer, std::ref(books.at(s)),
                                   std::ref(queues[s]), std::ref(total_trades),
                                   std::ref(per_consumer_latencies[i]));
        }
        for (auto& t : producers) t.join();
        for (const auto& s : symbols) queues[s].shutdown();
        for (auto& t : consumers) t.join();
        auto end = Clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

        std::vector<long> latencies;
        latencies.reserve(orders.size());
        for (auto& v : per_consumer_latencies)
            latencies.insert(latencies.end(), v.begin(), v.end());

        print_run(r, orders.size(), total_trades.load(), us);
        print_latency_stats(latencies);
        std::cout << "\n";
        write_run_csv(csv, "per_symbol_queue", r, orders.size(), us, latencies);
    }
}

void run_scaling_analysis(std::ofstream& csv) {
    std::cout << "\n==========SCALING ANALYSIS==========\n";
    std::cout << "Per-symbol order count: 250K (total scales with thread count)\n";

    std::vector<std::string> all_symbols = {"AAPL", "GOOGL", "MSFT", "TSLA",
                                             "AMZN", "META", "NVDA", "NFLX"};
    const int orders_per_symbol = 250000;
    OrderGenerator scale_gen(orders_per_symbol * (int)all_symbols.size(),
                             1000, 50, 1, 500, 0.6f, 0.2f, 0.2f, all_symbols);
    std::vector<Order> scale_orders = scale_gen.generate();

    SymOrders sorders;
    for (const auto& o : scale_orders) sorders[o.symbol].push_back(o);

    const int scale_runs = 3;

    std::cout << std::left
              << std::setw(9)  << "Symbols"
              << std::setw(12) << "Orders"
              << std::setw(18) << "Seq(orders/s)"
              << std::setw(18) << "Par(orders/s)"
              << std::setw(12) << "Speedup"
              << "Ideal\n";

    for (int n : {1, 2, 4, 8}) {
        std::vector<std::string> active(all_symbols.begin(), all_symbols.begin() + n);
        int total = 0;
        for (const auto& s : active) total += (int)sorders[s].size();

        double seq_best = 0;
        for (int r = 0; r < scale_runs; r++) {
            std::unordered_map<std::string, OrderBook> books;
            for (const auto& s : active) books.try_emplace(s, 950, 1050);
            auto t0 = Clock::now();
            for (const auto& s : active)
                for (const auto& o : sorders[s])
                    books.at(s).submit(o);
            auto t1 = Clock::now();
            double tput = total * 1000000.0 /
                std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
            seq_best = std::max(seq_best, tput);
        }

        double par_best = 0;
        for (int r = 0; r < scale_runs; r++) {
            std::unordered_map<std::string, OrderBook> books;
            for (const auto& s : active) books.try_emplace(s, 950, 1050);
            std::atomic<int> total_trades{0};
            std::vector<std::thread> threads;
            auto t0 = Clock::now();
            for (const auto& s : active) {
                threads.emplace_back([&books, &sorders, &total_trades, s]() {
                    int local = 0;
                    for (const auto& o : sorders[s]) {
                        auto trades = books.at(s).submit(o);
                        local += (int)trades.size();
                    }
                    total_trades += local;
                });
            }
            for (auto& t : threads) t.join();
            auto t1 = Clock::now();
            double tput = total * 1000000.0 /
                std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
            par_best = std::max(par_best, tput);
        }

        char speedup_buf[16];
        std::snprintf(speedup_buf, sizeof(speedup_buf), "%.2fx", par_best / seq_best);
        std::cout << std::left
                  << std::setw(9)  << n
                  << std::setw(12) << total
                  << std::setw(18) << (long long)seq_best
                  << std::setw(18) << (long long)par_best
                  << std::setw(12) << speedup_buf
                  << n << "x\n";

        csv << n << "," << total << ","
            << (long long)seq_best << "," << (long long)par_best << "\n";
    }
}

void run_workload_sweep(std::ofstream& csv) {
    std::cout << "\n==========WORKLOAD MIX SWEEP==========\n";
    std::cout << "Fixed: 4 symbols, 250K orders/symbol\n";

    std::vector<std::string> mix_symbols = {"AAPL", "GOOGL", "MSFT", "TSLA"};
    const int orders_per_sym = 250000;
    const int mix_runs = 3;

    struct Mix { const char* name; float limit_pct; float market_pct; float cancel_pct; };
    Mix mixes[] = {
        {"Limit-heavy  (80/10/10)", 0.80f, 0.10f, 0.10f},
        {"Default      (60/20/20)", 0.60f, 0.20f, 0.20f},
        {"Market-heavy (20/70/10)", 0.20f, 0.70f, 0.10f},
    };

    std::cout << std::left
              << std::setw(28) << "Mix"
              << std::setw(18) << "Seq(orders/s)"
              << std::setw(18) << "Par(orders/s)"
              << "Speedup\n";

    for (const auto& mix : mixes) {
        OrderGenerator mg(orders_per_sym * (int)mix_symbols.size(),
                          1000, 50, 1, 500,
                          mix.limit_pct, mix.market_pct, mix.cancel_pct,
                          mix_symbols);
        std::vector<Order> mix_orders = mg.generate();

        SymOrders msorders;
        for (const auto& o : mix_orders) msorders[o.symbol].push_back(o);
        int total = (int)mix_orders.size();

        double seq_best = 0;
        for (int r = 0; r < mix_runs; r++) {
            std::unordered_map<std::string, OrderBook> books;
            for (const auto& s : mix_symbols) books.try_emplace(s, 950, 1050);
            auto t0 = Clock::now();
            for (const auto& s : mix_symbols)
                for (const auto& o : msorders[s])
                    books.at(s).submit(o);
            auto t1 = Clock::now();
            double tput = total * 1000000.0 /
                std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
            seq_best = std::max(seq_best, tput);
        }

        double par_best = 0;
        for (int r = 0; r < mix_runs; r++) {
            std::unordered_map<std::string, OrderBook> books;
            for (const auto& s : mix_symbols) books.try_emplace(s, 950, 1050);
            std::atomic<int> total_trades{0};
            std::vector<std::thread> threads;
            auto t0 = Clock::now();
            for (const auto& s : mix_symbols) {
                threads.emplace_back([&books, &msorders, &total_trades, s]() {
                    int local = 0;
                    for (const auto& o : msorders[s]) {
                        auto trades = books.at(s).submit(o);
                        local += (int)trades.size();
                    }
                    total_trades += local;
                });
            }
            for (auto& t : threads) t.join();
            auto t1 = Clock::now();
            double tput = total * 1000000.0 /
                std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
            par_best = std::max(par_best, tput);
        }

        char speedup_buf[16];
        std::snprintf(speedup_buf, sizeof(speedup_buf), "%.2fx", par_best / seq_best);
        std::cout << std::left
                  << std::setw(28) << mix.name
                  << std::setw(18) << (long long)seq_best
                  << std::setw(18) << (long long)par_best
                  << speedup_buf << "\n";

        csv << mix.name << "," << (long long)seq_best << "," << (long long)par_best << "\n";
    }
}

// ─── main ───────────────────────────────────────────────────────────────────

int main() {
    std::vector<std::string> symbols = {"AAPL", "GOOGL", "MSFT", "TSLA"};

    OrderGenerator gen(1000000, 1000, 50, 1, 500, 0.6f, 0.2f, 0.2f, symbols);
    const std::vector<Order> orders = gen.generate();

    SymOrders symbol_orders;
    for (const auto& o : orders) symbol_orders[o.symbol].push_back(o);

    // Warmup
    {
        std::unordered_map<std::string, OrderBook> warmup_books;
        for (const auto& s : symbols) warmup_books.try_emplace(s, 950, 1050);
        for (const auto& s : symbols)
            for (const auto& o : symbol_orders[s])
                warmup_books.at(s).submit(o);
    }

    std::ofstream runs_csv("bench_runs.csv");
    runs_csv << "strategy,run,throughput,p50_ns,p95_ns,p99_ns\n";

    std::ofstream scaling_csv("bench_scaling.csv");
    scaling_csv << "n_symbols,total_orders,seq_tput,par_tput\n";

    std::ofstream workload_csv("bench_workload.csv");
    workload_csv << "mix,seq_tput,par_tput\n";

    const int runs = 5;

    run_sequential(symbols, orders, symbol_orders, runs, runs_csv);
    run_mutex(symbols, orders, runs, runs_csv);
    run_queue(symbols, orders, runs, runs_csv);
    run_per_symbol(symbols, orders, symbol_orders, runs, runs_csv);
    run_per_symbol_queue(symbols, orders, symbol_orders, runs, runs_csv);
    run_scaling_analysis(scaling_csv);
    run_workload_sweep(workload_csv);
}
