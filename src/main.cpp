#include <iostream>
#include "order_book.h"

int main() {
    OrderBook book;

    // Test 1: Partial fill
    std::cout << "=== Test 1: Partial Fill ===\n";
    Order o1{1, 50, 100, 1, Side::BID, OrderType::LIMIT};
    Order o2{2, 50, 60, 2, Side::ASK, OrderType::LIMIT};
    book.submit(o1);
    std::vector<Trade> t2 = book.submit(o2);
    for (const auto& x : t2) {
        std::cout << "Trade: bid_id=" << x.bid_order_id 
                  << " ask_id=" << x.ask_order_id
                  << " price=" << x.price 
                  << " qty=" << x.qty << "\n";
    }

    // Test 2: Multiple price levels
    std::cout << "\n=== Test 2: Multiple Price Levels ===\n";
    OrderBook book2;
    Order o3{1, 50, 100, 1, Side::BID, OrderType::LIMIT};
    Order o4{2, 49, 100, 2, Side::BID, OrderType::LIMIT};
    Order o5{3, 48, 100, 3, Side::BID, OrderType::LIMIT};
    Order o6{4, 0, 250, 4, Side::ASK, OrderType::MARKET};
    book2.submit(o3);
    book2.submit(o4);
    book2.submit(o5);
    std::vector<Trade> t6 = book2.submit(o6);
    for (const auto& x : t6) {
        std::cout << "Trade: bid_id=" << x.bid_order_id
                  << " ask_id=" << x.ask_order_id
                  << " price=" << x.price
                  << " qty=" << x.qty << "\n";
    }

    // Test 3: Cancel
    std::cout << "\n=== Test 3: Cancel ===\n";
    OrderBook book3;
    Order o7{1, 50, 100, 1, Side::BID, OrderType::LIMIT};
    Order o8{1, 0, 0, 2, Side::BID, OrderType::CANCEL};
    Order o9{2, 50, 100, 3, Side::ASK, OrderType::LIMIT};
    book3.submit(o7);
    book3.submit(o8);
    std::vector<Trade> t9 = book3.submit(o9);
    if (t9.empty()) {
        std::cout << "No trades — cancel worked correctly\n";
    } else {
        for (const auto& x : t9) {
            std::cout << "Trade: bid_id=" << x.bid_order_id
                      << " ask_id=" << x.ask_order_id
                      << " price=" << x.price
                      << " qty=" << x.qty << "\n";
        }
    }

    return 0;
}