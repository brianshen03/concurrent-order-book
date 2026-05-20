#ifndef TYPES_H
#define TYPES_H

#include <string>

enum class Side {BID, ASK};
enum class OrderType {LIMIT, MARKET, CANCEL};

struct Order {
    int id;
    int price;
    int qty;
    long timestamp;
    Side side;
    OrderType order_type;

    std::string symbol;
};

struct Trade {
    int bid_order_id;
    int ask_order_id;
    int price;
    int qty;
    long timestamp;
};

#endif
