#include "order_generator.h"
#include <random>

OrderGenerator::OrderGenerator(
    int num_orders, int mid_price, float spread,
    int min_qty, int max_qty,
    float limit_pct, float market_pct, float cancel_pct,
    std::vector<std::string> symbols)
    : num_orders(num_orders), mid_price(mid_price), spread(spread),
      min_qty(min_qty), max_qty(max_qty),
      limit_pct(limit_pct), market_pct(market_pct), cancel_pct(cancel_pct),
      symbols(symbols) {}


std::vector<Order> OrderGenerator::generate() {
    std::vector<Order> orders;
    std::mt19937 rng(std::random_device{}());
    
    std::uniform_int_distribution<int> price_dist(mid_price - spread, mid_price + spread);
    std::uniform_int_distribution<int> qty_dist(min_qty, max_qty);
    std::uniform_real_distribution<float> type_dist(0.0f, 1.0f);
    std::uniform_int_distribution<int> side_dist(0, 1);
    std::uniform_int_distribution<int> symbol_dist(0, symbols.size() - 1);

    int next_id = 1;

    for (int i = 0; i < num_orders; i++) {
        float roll = type_dist(rng);
        Order o;
        o.timestamp = i;

        if (!active_ids.empty() && roll >= limit_pct + market_pct) {
            // CANCEL — pick a random active order
            std::uniform_int_distribution<int> id_dist(0, active_ids.size() - 1);
            int idx = id_dist(rng);

            o.id = active_ids[idx].first;
            o.symbol = active_ids[idx].second;
            active_ids[idx] = active_ids.back();
            active_ids.pop_back();
            o.order_type = OrderType::CANCEL;
            o.side = Side::BID; // doesn't matter for cancel
            o.price = 0;
            o.qty = 0;

        } else if (roll < limit_pct) {
            // LIMIT
            o.symbol = symbols[symbol_dist(rng)];
            o.id = next_id++;
            o.order_type = OrderType::LIMIT;
            o.side = side_dist(rng) ? Side::BID : Side::ASK;
            o.price = price_dist(rng);
            o.qty = qty_dist(rng);
            active_ids.emplace_back(o.id, o.symbol);

        } else {
            // MARKET
            o.symbol = symbols[symbol_dist(rng)];
            o.id = next_id++;
            o.order_type = OrderType::MARKET;
            o.side = side_dist(rng) ? Side::BID : Side::ASK;
            o.price = 0;
            o.qty = qty_dist(rng);
        }

        orders.push_back(o);
    }
    return orders;
}