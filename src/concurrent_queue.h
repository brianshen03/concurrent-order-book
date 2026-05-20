#ifndef CONCURRENT_QUEUE_H
#define CONCURRENT_QUEUE_H

#include "types.h"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

class ConcurrentQueue {
public:
    void push(Order o) {
        { std::lock_guard<std::mutex> lock(mtx); q.push(o); }
        cv.notify_one();
    }

    // Blocks until an item is available or shutdown. Returns false when
    // shutdown and queue is empty (signals consumer to exit).
    bool pop(Order& o) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&]{ return !q.empty() || done.load(); });
        if (q.empty()) return false;
        o = q.front();
        q.pop();
        return true;
    }

    void shutdown() {
        done = true;
        cv.notify_all();
    }

private:
    std::queue<Order> q;
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> done{false};
};

#endif
