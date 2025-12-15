#pragma once

#include <functional>
#include <map>
#include <vector>

namespace tiny_mw {
namespace core {

class EventLoop {
public:
    using Callback = std::function<void()>;

    EventLoop();
    ~EventLoop();

    // 禁止拷贝 (EventLoop 独占 Epoll 资源)
    EventLoop(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;

    // 核心功能：注册一个 fd 和它对应的回调函数
    void addEvent(int fd, Callback cb);

    // 核心功能：开始循环
    void run();

    // 停止循环
    void stop();

private:
    int epoll_fd_;
    bool running_;
    
    // 存储 fd -> 回调函数的映射
    // 当 epoll 告诉我们 fd 活跃时，我们通过这个 map 找到对应的函数去执行
    std::map<int, Callback> callbacks_;
};

} // namespace core
} // namespace tiny_mw