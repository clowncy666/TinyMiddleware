#include "tiny_mw/node/Node.h"
#include "HelloWorldPubSubTypes.hpp"
#include <chrono>
#include <thread>
#include <iostream> // 引入 iostream

using namespace tiny_mw::node;

template<>
struct MsgTypeSupportTraits<HelloWorld> {
    using PubSubType = HelloWorldPubSubType;
};

int main() {
    auto node = std::make_shared<Node>("PerfPubNode");
    auto pub = node->create_publisher<HelloWorld>("PerfTopic");

    HelloWorld msg;
    // 稍微加长一点消息内容，模拟真实负载
    msg.message("123456789012345678901234567890"); 
    unsigned long index = 1;

    // ==========================================
    // 【测试参数】发送间隔 (微秒)
    // 修改这个值来寻找临界点：
    // 100 -> 比较保守，应该无丢包
    // 50  -> 压力增大
    // 10  -> 接近极限
    // ==========================================
    int sleep_us = 100; 

    std::cout << "Start publishing with interval: " << sleep_us << " us" << std::endl;

    while (true) {
        msg.index(index++);
        pub->publish(msg);

        // 控制发送速率
        std::this_thread::sleep_for(std::chrono::microseconds(sleep_us)); 
    }

    return 0;
}