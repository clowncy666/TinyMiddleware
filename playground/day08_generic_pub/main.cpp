#include <iostream>
#include <thread>
#include <string>
#include <chrono>

#include "tiny_mw/node/Node.h"
#include "HelloWorldPubSubTypes.hpp" 

// 别忘了特化 Traits！
template<>
struct MsgTypeSupportTraits<HelloWorld> {
    using PubSubType = HelloWorldPubSubType;
};

using namespace tiny_mw::node;

int main() {
    // 创建节点
    Node node("TalkerNode");

    // 1. 创建发布者
    auto pub = node.create_publisher<HelloWorld>("HelloWorldTopic");

    // 2. 启动一个线程，每秒发一次消息
    // (因为我们还没实现 Timer，暂时用线程模拟)
    std::thread publish_thread([pub]() {
        uint32_t count = 0;
        while(true) {
            HelloWorld msg;
            msg.index(count++);
            msg.message("Hello form TinyMW Generic Publisher!");

            // 发送！
            pub->publish(msg);
            
            std::cout << ">>> [Sent] Index: " << msg.index() << std::endl;
            
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });

    // 3. 同时创建一个订阅者，自己发自己收 (Loopback 测试)
    node.create_subscription<HelloWorld>("HelloWorldTopic", [](const HelloWorld& msg) {
        std::cout << "<<< [Recv] " << msg.message() 
                  << " | Index: " << msg.index() << std::endl;
    });

    // 启动循环
    node.spin();
    
    publish_thread.join();
    return 0;
}