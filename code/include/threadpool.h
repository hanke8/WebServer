#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include <thread>
#include <condition_variable>
#include <mutex>
#include <vector>
#include <queue>
#include <future>

class ThreadPool{
private:
    bool m_stop;
    std::vector<std::thread>m_thread;
    std::queue<std::function<void()>>tasks;
    std::mutex m_mutex;
    std::condition_variable m_cv;

public:
    explicit ThreadPool(size_t threadNumber):m_stop(false){
        for(size_t i=0;i<threadNumber;++i)
        {
            m_thread.emplace_back(
                [this](){
                    for(;;)
                    {
                        std::function<void()>task;
                        {
                            std::unique_lock<std::mutex>lk(m_mutex);
                            m_cv.wait(lk,[this](){ return m_stop||!tasks.empty();});
                            if(m_stop&&tasks.empty()) return;
                            task=std::move(tasks.front());
                            tasks.pop();
                        }
                        task();
                    }
                }
            );
        }
    }

    ThreadPool(const ThreadPool &) = delete;
    ThreadPool(ThreadPool &&) = delete;

    ThreadPool & operator=(const ThreadPool &) = delete;
    ThreadPool & operator=(ThreadPool &&) = delete;

    ~ThreadPool(){
        {
            std::unique_lock<std::mutex>lk(m_mutex);
            m_stop=true;
        }
        m_cv.notify_all();
        for(auto& threads:m_thread)
        {
            threads.join();
        }
    }

    template<typename F,typename... Args>
    auto submit(F&& f,Args&&... args)->std::future<decltype(f(args...))>{
        auto taskPtr=std::make_shared<std::packaged_task<decltype(f(args...))()>>(
            std::bind(std::forward<F>(f),std::forward<Args>(args)...)
        );
        {
            std::unique_lock<std::mutex>lk(m_mutex);
            if(m_stop) throw std::runtime_error("submit on stopped ThreadPool");
            tasks.emplace([taskPtr](){ (*taskPtr)(); });
        }
        m_cv.notify_one();
        return taskPtr->get_future();

    }
};

#endif

// #ifndef THREADPOOL_H
// #define THREADPOOL_H

// #include <mutex>
// #include <condition_variable>
// #include <queue>
// #include <thread>
// #include <functional>
// class ThreadPool {
// public:
//     explicit ThreadPool(size_t threadCount = 8): pool_(std::make_shared<Pool>()) {
//             assert(threadCount > 0);
//             for(size_t i = 0; i < threadCount; i++) {
//                 std::thread([pool = pool_] {
//                     std::unique_lock<std::mutex> locker(pool->mtx);
//                     while(true) {
//                         if(!pool->tasks.empty()) {
//                             auto task = std::move(pool->tasks.front());
//                             pool->tasks.pop();
//                             locker.unlock();
//                             task();
//                             locker.lock();
//                         } 
//                         else if(pool->isClosed) break;
//                         else pool->cond.wait(locker);
//                     }
//                 }).detach();
//             }
//     }

//     ThreadPool() = default;

//     ThreadPool(ThreadPool&&) = default;
    
//     ~ThreadPool() {
//         if(static_cast<bool>(pool_)) {
//             {
//                 std::lock_guard<std::mutex> locker(pool_->mtx);
//                 pool_->isClosed = true;
//             }
//             pool_->cond.notify_all();
//         }
//     }

//     template<class F>
//     void submit(F&& task) {
//         {
//             std::lock_guard<std::mutex> locker(pool_->mtx);
//             pool_->tasks.emplace(std::forward<F>(task));
//         }
//         pool_->cond.notify_one();
//     }

// private:
//     struct Pool {
//         std::mutex mtx;
//         std::condition_variable cond;
//         bool isClosed;
//         std::queue<std::function<void()>> tasks;
//     };
//     std::shared_ptr<Pool> pool_;
// };


// #endif //THREADPOOL_H