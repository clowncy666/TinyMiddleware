#include "tiny_mw/core/ThreadPool.h"
#include <iostream>
#include <chrono>
#include <random>
using namespace tiny_mw::core;
int main (){
    std::cout << "create threadpool with 4 workers" << std::endl;
    ThreadPool pool(4);
    for (int i = 0; i < 10; ++i) {
        // 使用 Lambda 表达式封装任务
        pool.enqueue([i] {
            // 获取当前线程 ID 用于打印
            std::thread::id this_id = std::this_thread::get_id();
            std::cout << "[Worker " << this_id << "] Start processing task " << i << std::endl;
            // 模拟耗时操作
            std::this_thread::sleep_for(std::chrono::milliseconds(200 + i * 10));
            std::cout << "[Worker " << this_id << "] Finished task " << i << std::endl;
        });
    }
    std::cout << "All tasks have been enqueued." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::cout << "Main thread is exiting." << std::endl;
    return 0;
}
