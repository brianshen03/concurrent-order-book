#include "order_book.h"


std::vector<Trade> OrderBook::submit(Order o) {

    OrderType order_type = o.order_type;

    switch(order_type) {
        case OrderType::LIMIT:
            return order_limit(o);
        case OrderType::MARKET:
            return order_market(o);
        case OrderType::CANCEL:
            order_cancel(o.id);
            break;
        default:
            return {};
    }
    return {};
}

//core matching logic 
std::vector<Trade> OrderBook::order_match(Order& o) {

    int remaining_qty = o.qty;
    std::vector<Trade> trades;
    Side side = o.side;
    int price = o.price;
    

    if (side == Side::BID) {

        auto it = asks.begin();

        while (remaining_qty > 0 && !asks.empty() && it->first <= price) {
            int filled = std::min(remaining_qty, it->second.front().qty);
            remaining_qty -= filled;
            it->second.front().qty -= filled;

            //update order map
            if (it->second.front().qty == 0) {
                order_map.erase(it->second.front().id);
            } else {
                order_map[it->second.front().id].qty -= filled;
            }
            
            //record trade and push it into vector 
            Trade t;
            t.ask_order_id = it->second.front().id;
            t.bid_order_id = o.id;
            t.price = it->second.front().price;
            t.qty = filled;
            t.timestamp = o.timestamp;
            trades.push_back(t);

            //move onto next order at price level OR next price level 
            if (it->second.front().qty == 0) {
                it->second.pop();

                //if exists another entry in queue, move onto that entry
                if (it->second.empty()) {
                    it = asks.erase(it);
                }
                //else move onto next price level 
            }
        }

        if (remaining_qty > 0) {

        }
    } else {

        auto it = bids.begin();

        while (remaining_qty > 0 && !bids.empty() && it->first >= price) {
            int filled = std::min(remaining_qty, it->second.front().qty);
            remaining_qty -= filled;
            it->second.front().qty -= filled;

            if (it->second.front().qty == 0) {
                order_map.erase(it->second.front().id);
            } else {
                order_map[it->second.front().id].qty -= filled;
            }
            
            Trade t;
            t.ask_order_id = o.id;
            t.bid_order_id = it->second.front().id;
            t.price = it->second.front().price;
            t.qty = filled;
            t.timestamp = o.timestamp;
            trades.push_back(t);

            if (it->second.front().qty == 0) {
                it->second.pop();

                if (it->second.empty()) {
                    it = bids.erase(it);
                }
            }
        }

    }
    
    return trades;
}


//Limit Order
// "I want to buy/sell X shares, but only at a specific price or better."
std::vector<Trade> OrderBook::order_limit(Order o) {

    int price = o.price;
    Side side = o.side;
    std::vector<Trade> trades;

    if (side == Side::BID) {
        //check if there is a matching price on other side 
        auto it = asks.begin();

        //YES - can be matched 
        if (!asks.empty() && it->first <= price) {
            trades = order_match(o);
            if (o.qty > 0) {
                bids[price].push(o);
                order_map[o.id] = o;
            }
        } else {
            //can NOT be matched, add to buy order book 
            //find right price level and add to back of queue 
            bids[price].push(o);
            order_map[o.id] = o;
        }
    } else {
        auto it = bids.begin();
        if (!bids.empty() && it->first >= price) {
            trades = order_match(o);
            if (o.qty > 0) {
                asks[price].push(o);
                order_map[o.id] = o;
            }
        } else {
            asks[price].push(o);
            order_map[o.id] = o;
        }
    }

    return trades;
}

//Market Order
// "I want to buy/sell X shares right now at whatever price is available."
std::vector<Trade> OrderBook::order_market(Order o) {

    Side side = o.side;

    if (side == Side::BID) {
        o.price = INT_MAX;
    } else {
        o.price = 0;
    }

    return order_match(o);
}

//cancel order 
//removes existing order from the book 
void OrderBook::order_cancel(int order_id) {

    //if order_id does not exist 
    if (order_map.find(order_id) == order_map.end()) {
        return;
    }

    //remove order from id map 
    Order o = order_map[order_id];
    order_map.erase(order_id);

    Side side = o.side;
    int price = o.price;

    if (side == Side::BID) {
        //find element in queue and remove it 
        std::queue<Order> temp;
        while (!bids[price].empty()) {
            Order current = bids[price].front();
            bids[price].pop();
            if (current.id != o.id) {
                temp.push(current);
            }

        }
        //repopulate original queue
        if (temp.empty()) {
            bids.erase(price);
        } else bids[price] = temp;
    } else {
        std::queue<Order> temp;
        while (!asks[price].empty()) {
            Order current = asks[price].front();
            asks[price].pop();

            if (current.id != o.id) {
                temp.push(current);
            }
        }
        //repopulate original queue
        if (temp.empty()) {
            asks.erase(price);
        } else asks[price] = temp;
    }
}






