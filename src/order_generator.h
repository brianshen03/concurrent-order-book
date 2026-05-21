#ifndef ORDER_GENERATOR_H
#define ORDER_GENERATOR_H

#include "types.h"
#include <vector>
#include <unordered_map>


class OrderGenerator {
    public:
        std::vector<Order> generate();
        OrderGenerator(int num_orders, int mid_price, float spread,
               int min_qty, int max_qty,
               float limit_pct, float market_pct, float cancel_pct, std::vector<std::string> symbols);
    private:
        int num_orders;
        int mid_price;     // center of the uniform price distribution
        float spread;      // half-width: prices drawn from [mid - spread, mid + spread]
        int min_qty;
        int max_qty;
        float limit_pct;
        float market_pct;
        float cancel_pct;

        std::vector<std::string> symbols;
        // Tracks live limit order (id, symbol) pairs so cancel orders target real orders.
        // Only limit orders are tracked — market orders never rest in the book.
        std::vector<std::pair<int, std::string>> active_ids;

};

#endif