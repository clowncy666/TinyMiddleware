#include <iostream>
#include <string>
#include <thread>

#include "tiny_mw/node/Node.h"
#include "HelloWorldPubSubTypes.hpp" 

// Traits 特化
template<>
struct MsgTypeSupportTraits<HelloWorld> {
    using PubSubType = HelloWorldPubSubType;
};

using namespace tiny_mw::node;

int main() {
    Node node("TimerBot");

    // 1. 创建发布者
    auto pub = node.create_publisher<HelloWorld>("SensorData");

    // 2. [关键] 创建 500ms 定时器用于发送数据
    // 注意：我们需要用 static 或者 lambda capture 来保存 count 状态
    auto timer_pub = node.create_wall_timer(500, [pub]() {
        static uint32_t count = 0;
        
        HelloWorld msg;
        msg.index(count++);
        msg.message("Sensor Reading");
        
        pub->publish(msg);
        // std::cout << ">>> [Timer] Published index: " << count-1 << std::endl;
    });

    // 3. 创建 2000ms 定时器做系统自检
    auto timer_check = node.create_wall_timer(2000, []() {
        std::cout << "[System] 2 seconds heartbeat check... OK" << std::endl;
    });

    // 4. 创建订阅者
    node.create_subscription<HelloWorld>("SensorData", [](const HelloWorld& msg) {
        std::cout << "<<< [Recv] " << msg.message() 
                  << " | Index: " << msg.index() 
                  << " | Thread: " << std::this_thread::get_id() << std::endl;
    });

    // 5. 开始运行
    // 此时主线程阻塞在这里，但 EventLoop 会同时处理 Timer事件 和 DDS网络事件
    node.spin();

    return 0;
}