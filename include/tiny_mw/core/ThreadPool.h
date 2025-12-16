#pragma once
#include <vector>
#include <thread>
#include <functional>
#include <queue>
#include <condition_variable>
namespace tiny_mw {
namespace core {
class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads);
    ~ThreadPool();
    void enqueue(const std::function<void()>& task);
private:
    void worker_thread();
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    bool stop_;
};
}
}


