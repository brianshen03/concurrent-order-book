#include "order_book.h"
#include <algorithm>
#include <climits>

OrderBook::OrderBook(int min_price, int max_price)
    : min_price(min_price), max_price(max_price),
      bids(max_price - min_price + 1),
      asks(max_price - min_price + 1) {}

std::vector<Trade> OrderBook::submit(Order o) {
    switch (o.order_type) {
        case OrderType::LIMIT:  return order_limit(o);
        case OrderType::MARKET: return order_market(o);
        case OrderType::CANCEL: order_cancel(o.id); break;
        default: return {};
    }
    return {};
}

std::vector<Trade> OrderBook::order_match(Order& o) {
    std::vector<Trade> trades;

    if (o.side == Side::BID) {
        // Buy: sweep asks from cheapest up to the bid's limit price.
        int p_max = std::min(o.price, max_price);  // clamp to array bounds
        for (int p = min_price; p <= p_max && o.qty > 0; p++) {
            auto& level = asks[p - min_price];
            while (o.qty > 0 && !level.empty()) {
                Order& resting = level.front();
                int filled = std::min(o.qty, resting.qty);
                o.qty -= filled;
                resting.qty -= filled;  // modify queue element in-place

                Trade t;
                t.ask_order_id = resting.id;
                t.bid_order_id = o.id;
                t.price = resting.price;
                t.qty = filled;
                t.timestamp = o.timestamp;
                trades.push_back(t);

                if (resting.qty == 0) {
                    order_map.erase(resting.id);
                    level.pop();
                } else {
                    // Queue element and map entry are separate copies; sync the map.
                    order_map[resting.id].qty -= filled;
                }
            }
        }
    } else {
        // Sell: sweep bids from most generous down to the ask's limit price.
        int p_min = std::max(o.price, min_price);  // clamp to array bounds
        for (int p = max_price; p >= p_min && o.qty > 0; p--) {
            auto& level = bids[p - min_price];
            while (o.qty > 0 && !level.empty()) {
                Order& resting = level.front();
                int filled = std::min(o.qty, resting.qty);
                o.qty -= filled;
                resting.qty -= filled;

                Trade t;
                t.ask_order_id = o.id;
                t.bid_order_id = resting.id;
                t.price = resting.price;
                t.qty = filled;
                t.timestamp = o.timestamp;
                trades.push_back(t);

                if (resting.qty == 0) {
                    order_map.erase(resting.id);
                    level.pop();
                } else {
                    order_map[resting.id].qty -= filled;
                }
            }
        }
    }

    return trades;
}

std::vector<Trade> OrderBook::order_limit(Order o) {
    std::vector<Trade> trades = order_match(o);
    if (o.qty > 0) {
        int idx = o.price - min_price;
        if (o.side == Side::BID)
            bids[idx].push(o);
        else
            asks[idx].push(o);
        order_map[o.id] = o;
    }
    return trades;
}

std::vector<Trade> OrderBook::order_market(Order o) {
    // Sentinel prices ensure the order sweeps the entire book.
    o.price = (o.side == Side::BID) ? INT_MAX : 0;
    return order_match(o);
}

void OrderBook::order_cancel(int order_id) {
    auto it = order_map.find(order_id);
    if (it == order_map.end()) return;

    Order o = it->second;
    order_map.erase(it);

    int idx = o.price - min_price;
    auto& level = (o.side == Side::BID) ? bids[idx] : asks[idx];

    // std::queue has no erase, so rebuild the level without the cancelled order.
    // O(n) in the depth of that price level — acceptable since cancels are rare
    // relative to matches and levels are typically shallow.
    std::queue<Order> temp;
    while (!level.empty()) {
        Order current = level.front();
        level.pop();
        if (current.id != o.id) temp.push(current);
    }
    level = std::move(temp);
}
