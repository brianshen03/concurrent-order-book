#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H

#include "types.h"
#include <queue>
#include <unordered_map>
#include <vector>

// Single-threaded. Callers are responsible for external synchronization.
class OrderBook {
public:
    OrderBook(int min_price, int max_price);
    std::vector<Trade> submit(Order o);

private:
    int min_price;
    int max_price;

    // Price ladder: indexed by (price - min_price). Each slot is a FIFO queue
    // of resting orders at that price. Bids matched high→low, asks low→high.
    std::vector<std::queue<Order>> bids;
    std::vector<std::queue<Order>> asks;
    std::unordered_map<int, Order> order_map;  // O(1) lookup for cancels

    std::vector<Trade> order_limit(Order o);
    std::vector<Trade> order_market(Order o);
    std::vector<Trade> order_match(Order& o);
    void order_cancel(int order_id);

};

#endif
