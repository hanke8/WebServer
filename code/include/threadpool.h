#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include <assert.h>
#include <vector>
#include <queue>
#include <unordered_map>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
using namespace std;

class ThreadPool {
public:
    ThreadPool(int size) {
        assert(size > 0);
        isClose_ = false;
        free_ = tot_ = 0;
        mxFree_ = size;

        for(int i = 0; i < size; i++) {
            {
                unique_lock<mutex> lck(mtx_);
                free_++;
                tot_++;
            }
            increaseThread();
        }
    }

    ThreadPool(const ThreadPool &) = delete;
    ThreadPool(ThreadPool &&) = delete;

    ThreadPool & operator=(const ThreadPool &) = delete;
    ThreadPool & operator=(ThreadPool &&) = delete;

    ~ThreadPool() {
        {
            unique_lock<mutex> lck(mtx_);
            isClose_ = true;
        }
        cond_.notify_all();
        for(int i = 0; i < tot_; i++) {
            threads_[i].join();
        }
    }

    // 该线程池，可处理任意长度，任意类型参数的任务
    template<class F, class... Args>
    auto submit(F&& f, Args&&... args)->decltype(f(args...)) {
        function<decltype(f(args...))()> task = bind(forward<F>(f), forward<Args>(args)...);
        auto taskPtr = make_shared<packaged_task<decltype(f(args...))()>>(task);
        {
            unique_lock<mutex> lck(mtx_);
            tasks_.push(
                [taskPtr]() {
                    (*taskPtr)();
                }
            );
            
            if(free_ == 0) {
                free_++;
                tot_++;
                increaseThread();
            }
        }
        cond_.notify_one();
        return taskPtr->get_future().get();
    }

private:
    bool isClose_;
    int free_, tot_, mxFree_;
    mutex mtx_;
    condition_variable cond_;
    vector<thread> threads_;
    unordered_map<thread::id, bool> isFree_;
    queue<function<void()>> tasks_;

    void increaseThread() {
        threads_.emplace_back(
            [this]() {
                while(1) {
                    function<void()> task;
                    {
                        unique_lock<mutex> lck(mtx_);
                        cond_.wait(lck, [this]() {return isClose_ || !tasks_.empty();});
                        if(isClose_ && tasks_.empty()) break;
                        task = tasks_.front();
                        tasks_.pop();
                        free_--;
                    }
                    task();
                    {
                        unique_lock<mutex> lck(mtx_);
                        free_++;
                        if(free_ > mxFree_) {           // 减容
                            free_--;
                            return;
                        }
                    }
                }
            }
        );
    }

};

#endif
