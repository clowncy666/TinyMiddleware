#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <iostream>     // 修复: std::cout, std::endl
#include <thread>       // 修复: std::thread, std::this_thread
#include <chrono>       // 修复: std::chrono
#include <cstdio>       // 修复: perror
void worker_thread_func(int efd) {
    uint64_t u = 1; 
    while (true) {
        
        std::this_thread::sleep_for(std::chrono::seconds(1));

        
        ssize_t s = write(efd, &u, sizeof(uint64_t));
        if (s != sizeof(uint64_t)) {
            perror("write error");
        }
        
    }
}

int main(){
    int efd=eventfd(0, EFD_NONBLOCK);
    if(efd==-1)return -1;
    std::thread t(worker_thread_func, efd);
    t.detach();
    int epfd=epoll_create1(0);
    if(epfd==-1){perror("epoll_create1");return -1;}
    struct epoll_event ev;
    ev.events=EPOLLIN;
    ev.data.fd=efd;
    if(epoll_ctl(epfd, EPOLL_CTL_ADD, efd, &ev)==-1){perror("epoll_ctl");return -1;}
    const int MAX_EVENTS = 10;
    struct epoll_event events[MAX_EVENTS];
    while (true) {
    int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
    if (nfds == -1) {
        perror("epoll_wait");
        return -1;
    }
    for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == efd) {
                std::cout << "Woke up!" << std::endl;
                uint64_t u;
                ssize_t s = read(efd, &u, sizeof(uint64_t));
                if (s != sizeof(uint64_t)) {
                    perror("read error");
                }
            }
        }
    }
    close(efd);
    close(epfd);
    return 0;
}           