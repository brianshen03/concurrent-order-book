#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H

#include "types.h"
#include <queue>
#include <unordered_map>
#include <vector>

class OrderBook {
public:
    OrderBook(int min_price, int max_price);
    std::vector<Trade> submit(Order o);

private:
    int min_price;
    int max_price;

    // indexed by price - min_price; bids iterated high→low, asks low→high
    std::vector<std::queue<Order>> bids;
    std::vector<std::queue<Order>> asks;
    std::unordered_map<int, Order> order_map;

    std::vector<Trade> order_limit(Order o);
    std::vector<Trade> order_market(Order o);
    std::vector<Trade> order_match(Order& o);
    void order_cancel(int order_id);

};

#endif
