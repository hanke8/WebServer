#include "timer.h"

void HeapTimer::add(int id, int timeout, const std::function<void()>& cb) {
    assert(id >= 0);
    size_t i;
    if(mp_.count(id) == 0) {
        // 新节点：堆尾插入，向上调整堆
        i = minheap_.size();
        mp_[id] = i;
        minheap_.push_back({id, Clock::now() + MS(timeout), cb});
        siftUp(i);
    } 
    else {
        // 已有结点：调整堆
        i = mp_[id];
        minheap_[i].expire = Clock::now() + MS(timeout);
        minheap_[i].cb = cb;
        siftDown(i, minheap_.size());
    }
}

void HeapTimer::reset(int id, int timeout) {
    assert(!minheap_.empty() && mp_.count(id) > 0);
    minheap_[mp_[id]].expire = Clock::now() + MS(timeout);
    siftDown(mp_[id], minheap_.size());
}

int HeapTimer::tick() {
    // 处理已经超时的事件
    while(!minheap_.empty()) {
        TimerNode node = minheap_.front();
        if(std::chrono::duration_cast<MS>(node.expire - Clock::now()).count() > 0) { 
            break; 
        }
        node.cb();
        pop();
    }

    // 下一个到期时间
    size_t nextTimeOut = -1;
    if(!minheap_.empty()) {
        nextTimeOut = std::chrono::duration_cast<MS>(minheap_.front().expire - Clock::now()).count();
        if(nextTimeOut < 0) nextTimeOut = 0;
    }
    return nextTimeOut;
}

void HeapTimer::pop() {
    assert(!minheap_.empty());
    size_t i = 0, n = minheap_.size() - 1;
    // 将堆头（要删除的结点）与堆尾交换，再向下调整
    swapNode(0, n);
    siftDown(0, n);
    mp_.erase(minheap_.back().id);
    minheap_.pop_back();
}

void HeapTimer::siftUp(size_t i) {
    assert(i >= 0 && i < minheap_.size());
    size_t j = (i - 1) / 2;
    while(i > 0) {
        j = (i - 1) / 2;
        if(minheap_[i].expire > minheap_[j].expire) break;
        swapNode(i, j);
        i = j;
    }
}

void HeapTimer::siftDown(size_t i, size_t n) {
    assert(i >= 0 && i < minheap_.size());
    assert(n >= 0 && n <= minheap_.size());
    size_t j = 2 * i + 1;
    while(j < n) {
        if(j + 1 < n && minheap_[j+1].expire < minheap_[j].expire) j++;
        if(minheap_[i].expire < minheap_[j].expire) break;
        swapNode(i, j);
        i = j;
        j = 2 * i + 1;
    }
}

void HeapTimer::swapNode(size_t i, size_t j) {
    assert(i >= 0 && i < minheap_.size());
    assert(j >= 0 && j < minheap_.size());
    std::swap(minheap_[i], minheap_[j]);
    mp_[minheap_[i].id] = i;
    mp_[minheap_[j].id] = j;
}
