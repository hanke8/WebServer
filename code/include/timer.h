#ifndef TIMER_H_
#define TIMER_H_

#include <ctime>
#include <chrono>
#include <functional>
#include <unordered_map>
#include <vector>
#include <assert.h> 

#include "httpconnection.h"

typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;

struct TimerNode {
    int id;
    TimeStamp expire;               // 过期时间
    std::function<void()> cb;       // 回调函数
};

class HeapTimer {
public:
    HeapTimer() {}
    ~HeapTimer() {}

    void add(int id, int timeout, const std::function<void()>& cb);
    void reset(int id, int timeout);
    int tick();

private:
    void pop();
    void siftUp(size_t i);
    void siftDown(size_t i, size_t n);
    void swapNode(size_t i, size_t j);

    std::vector<TimerNode> minheap_;
    std::unordered_map<int, size_t> mp_;   // id, index
};

#endif