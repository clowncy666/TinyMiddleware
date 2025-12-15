#include "tiny_mw/core/EventLoop.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <iostream>
#include <vector>

namespace tiny_mw {
namespace core {

EventLoop::EventLoop() : running_(false) {
    // 1. 创建 epoll 句柄
    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ == -1) {
        perror("[EventLoop] epoll_create1 failed");
        exit(EXIT_FAILURE);
    }
}

EventLoop::~EventLoop() {
    if (epoll_fd_ != -1) {
        close(epoll_fd_);
    }
}

void EventLoop::addEvent(int fd, Callback cb) {
    // 1. 将 callback 存入 map
    callbacks_[fd] = cb;

    // 2. 将 fd 注册到 kernel epoll
    struct epoll_event ev;
    ev.events = EPOLLIN; // 默认只监听可读事件
    ev.data.fd = fd;
    
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) == -1) {
        perror("[EventLoop] epoll_ctl add failed");
        // 实际工程中这里应该抛异常或返回错误码
    }
}

void EventLoop::stop() {
    running_ = false;
}

void EventLoop::run() {
    running_ = true;
    const int MAX_EVENTS = 16;
    std::vector<struct epoll_event> events(MAX_EVENTS);

    std::cout << "[EventLoop] Loop started..." << std::endl;

    while (running_) {
        // -1 表示无限等待，直到有事件发生
        int n = epoll_wait(epoll_fd_, events.data(), MAX_EVENTS, -1);

        if (n == -1) {
            if (errno == EINTR) continue; // 被信号打断
            perror("[EventLoop] epoll_wait failed");
            break;
        }

        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;
            
            // 查找并执行回调
            if (callbacks_.find(fd) != callbacks_.end()) {
                callbacks_[fd]();
            }
        }
    }
    std::cout << "[EventLoop] Loop stopped." << std::endl;
}

} // namespace core
} // namespace tiny_mw