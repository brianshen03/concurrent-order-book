#include <iostream>
#include <chrono>
#include "order_book.h"
#include "order_generator.h"

int main() {
    // Generate 1M orders
    OrderGenerator gen(1000000, 1000, 50, 1, 500, 0.6f, 0.2f, 0.2f);
    std::vector<Order> orders = gen.generate();

    // Warmup run - don't measure
    {
        OrderBook warmup_book;
        for (const auto& o : orders) {
            warmup_book.submit(o);
        }
    }

    // Measured runs
    int runs = 5;
    for (int r = 0; r < runs; r++) {
        OrderBook book;
        int total_trades = 0;

        auto start = std::chrono::high_resolution_clock::now();

        for (const auto& o : orders) {
            std::vector<Trade> trades = book.submit(o);
            total_trades += trades.size();
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        std::cout << "Run " << r + 1 << ": "
                  << "Orders=" << orders.size() << " "
                  << "Trades=" << total_trades << " "
                  << "Time=" << duration.count() << " us "
                  << "Throughput=" << (orders.size() * 1000000.0 / duration.count()) << " orders/sec\n";
    }

    return 0;
}