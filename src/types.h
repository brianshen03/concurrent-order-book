#ifndef TYPES_H
#define TYPES_H

#include <string>

enum class Side {BID, ASK};
enum class OrderType {LIMIT, MARKET, CANCEL};

struct Order {
    int id;
    int price;   // ignored for MARKET (set to INT_MAX/0 internally) and CANCEL
    int qty;     // ignored for CANCEL
    long timestamp;
    Side side;
    OrderType order_type;

    std::string symbol;  // used externally to route to the correct OrderBook
};

struct Trade {
    int bid_order_id;
    int ask_order_id;
    int price;   // execution price — the resting order's price (price-time priority)
    int qty;
    long timestamp;
};

#endif
