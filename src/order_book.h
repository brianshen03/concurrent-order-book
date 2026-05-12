#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H

#include "types.h"
#include <map>
#include <queue>
#include <unordered_map>
#include <vector>

class OrderBook {
public:
    std::vector<Trade> submit(Order o);

private:
    //buy orders, sorted by price descending
    std::map<int, std::queue<Order>, std::greater<int>> bids;
    //sell orders, sorted by price ascending 
    std::map<int, std::queue<Order>> asks;
    //specifically for cancel orders, avoid traversing entire bids/asks map - O(n), can go straight to right price level 
    std::unordered_map<int, Order> order_map;
    int next_id;

    std::vector<Trade> order_limit(Order o);
    std::vector<Trade> order_market(Order o);
    std::vector<Trade> order_match(Order& o);
    void order_cancel(int order_id);
};

#endif