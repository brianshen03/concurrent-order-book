#include <iostream>
#include <cassert>
#include "order_book.h"

void print_trades(const std::vector<Trade>& trades) {
    if (trades.empty()) {
        std::cout << "No trades\n";
        return;
    }
    for (const auto& t : trades) {
        std::cout << "  Trade: bid_id=" << t.bid_order_id
                  << " ask_id=" << t.ask_order_id
                  << " price=" << t.price
                  << " qty=" << t.qty << "\n";
    }
}

void test_basic_match() {
    std::cout << "=== Test 1: Basic Match ===\n";
    OrderBook book(0, 200);
    Order o1{1, 50, 100, 1, Side::BID, OrderType::LIMIT};
    Order o2{2, 50, 100, 2, Side::ASK, OrderType::LIMIT};
    book.submit(o1);
    auto trades = book.submit(o2);
    print_trades(trades);
    assert(trades.size() == 1);
    assert(trades[0].qty == 100);
    assert(trades[0].price == 50);
    std::cout << "PASSED\n\n";
}

void test_partial_fill() {
    std::cout << "=== Test 2: Partial Fill ===\n";
    OrderBook book(0, 200);
    Order o1{1, 50, 100, 1, Side::BID, OrderType::LIMIT};
    Order o2{2, 50, 60, 2, Side::ASK, OrderType::LIMIT};
    book.submit(o1);
    auto trades = book.submit(o2);
    print_trades(trades);
    assert(trades.size() == 1);
    assert(trades[0].qty == 60);
    // submit another sell to consume remaining 40
    Order o3{3, 50, 40, 3, Side::ASK, OrderType::LIMIT};
    auto trades2 = book.submit(o3);
    print_trades(trades2);
    assert(trades2.size() == 1);
    assert(trades2[0].qty == 40);
    std::cout << "PASSED\n\n";
}

void test_multiple_price_levels() {
    std::cout << "=== Test 3: Multiple Price Levels ===\n";
    OrderBook book(0, 200);
    Order o1{1, 50, 100, 1, Side::BID, OrderType::LIMIT};
    Order o2{2, 49, 100, 2, Side::BID, OrderType::LIMIT};
    Order o3{3, 48, 100, 3, Side::BID, OrderType::LIMIT};
    Order o4{4, 0, 250, 4, Side::ASK, OrderType::MARKET};
    book.submit(o1);
    book.submit(o2);
    book.submit(o3);
    auto trades = book.submit(o4);
    print_trades(trades);
    assert(trades.size() == 3);
    assert(trades[0].qty == 100);
    assert(trades[1].qty == 100);
    assert(trades[2].qty == 50);
    std::cout << "PASSED\n\n";
}

void test_cancel() {
    std::cout << "=== Test 4: Cancel ===\n";
    OrderBook book(0, 200);
    Order o1{1, 50, 100, 1, Side::BID, OrderType::LIMIT};
    Order o2{1, 0, 0, 2, Side::BID, OrderType::CANCEL};
    Order o3{2, 50, 100, 3, Side::ASK, OrderType::LIMIT};
    book.submit(o1);
    book.submit(o2);
    auto trades = book.submit(o3);
    print_trades(trades);
    assert(trades.empty());
    std::cout << "PASSED\n\n";
}

void test_market_order() {
    std::cout << "=== Test 5: Market Order ===\n";
    OrderBook book(0, 200);
    Order o1{1, 50, 100, 1, Side::BID, OrderType::LIMIT};
    Order o2{2, 49, 100, 2, Side::BID, OrderType::LIMIT};
    Order o3{3, 0, 150, 3, Side::ASK, OrderType::MARKET};
    book.submit(o1);
    book.submit(o2);
    auto trades = book.submit(o3);
    print_trades(trades);
    assert(trades.size() == 2);
    assert(trades[0].qty == 100);
    assert(trades[1].qty == 50);
    std::cout << "PASSED\n\n";
}

void test_no_match_price() {
    std::cout << "=== Test 6: No Match Due To Price ===\n";
    OrderBook book(0, 200);
    Order o1{1, 48, 100, 1, Side::BID, OrderType::LIMIT};
    Order o2{2, 50, 100, 2, Side::ASK, OrderType::LIMIT};
    book.submit(o1);
    auto trades = book.submit(o2);
    print_trades(trades);
    assert(trades.empty());
    std::cout << "PASSED\n\n";
}

int main() {
    test_basic_match();
    test_partial_fill();
    test_multiple_price_levels();
    test_cancel();
    test_market_order();
    test_no_match_price();
    std::cout << "All tests passed\n";
    return 0;
}