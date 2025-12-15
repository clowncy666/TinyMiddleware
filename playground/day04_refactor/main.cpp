#include "tiny_mw/core/EventLoop.h"
#include <sys/eventfd.h>
#include <unistd.h>
#include <thread>
#include <iostream>
using namespace tiny_mw::core;
int main ()
{
    int efd = eventfd (0, EFD_NONBLOCK | EFD_CLOEXEC);
    EventLoop loop;
    loop.addEvent(efd,[efd](){
        uint64_t u;
        read(efd, &u, sizeof(u));
        std::cout << "[Callback] EventFD triggered! Value: " << u << std::endl;
    });
    std::thread t([efd](){
        for (int i = 1; i <= 3; i++) {
            sleep(1);
            uint64_t u = 1;
            write(efd, &u, sizeof(u));
            std::cout << "[Thread] Sent signal." << std::endl;
        }
    });
    t.detach();
    loop.run();
    return 0;
}