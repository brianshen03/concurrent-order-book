#ifndef CONCURRENT_QUEUE_H
#define CONCURRENT_QUEUE_H

#include "types.h"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

// MPMC blocking queue. Intended use: multiple producer threads push orders;
// one or more consumer threads pop until shutdown() is called.
class ConcurrentQueue {
public:
    void push(Order o) {
        { std::lock_guard<std::mutex> lock(mtx); q.push(o); }
        cv.notify_one();
    }

    // Blocks until an item is available or shutdown is signalled.
    // Returns false only when the queue is empty and shutdown has been called —
    // the consumer's cue to exit.
    bool pop(Order& o) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&]{ return !q.empty() || done.load(); });
        if (q.empty()) return false;
        o = q.front();
        q.pop();
        return true;
    }

    // One-way: once shut down the queue cannot be restarted.
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
