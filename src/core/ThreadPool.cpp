#include "tiny_mw/core/ThreadPool.h"
#include <iostream>
namespace tiny_mw{
namespace core{
ThreadPool::ThreadPool(size_t num_threads): stop_(false) {
    for (size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back(&ThreadPool::worker_thread, this);
    }
}
ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        stop_ = true;
    }
    condition_.notify_all();
    for (std::thread &worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}
void ThreadPool::enqueue(const std::function<void()>& task) {
    {   
        std::unique_lock<std::mutex> lock(queue_mutex_);
        if (stop_) return;
        tasks_.push(task);
    }
    condition_.notify_one();
}
void ThreadPool::worker_thread() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            condition_.wait(lock, [this] { return this->stop_ || !this->tasks_.empty();
            });
        if(this->stop_ && this->tasks_.empty()) return;
        task = std::move(this->tasks_.front());
        this->tasks_.pop();
        }
        task();
    }
}
}
}