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
        //price distribution to center around 
        int mid_price;
        float spread;
        //min and max qty for each order type 
        int min_qty;
        int max_qty;
        //what percentage of all orders are limit, market, and cancel orders 
        float limit_pct;
        float market_pct;
        float cancel_pct;

        std::vector<std::string> symbols;
        //tracks which limit orders are live, so the generator can produce valid cancel orders 
        std::vector<std::pair<int, std::string>> active_ids;

};

#endif