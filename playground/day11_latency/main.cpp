#include <iostream>
#include <chrono>
#include <thread>
#include "tiny_mw/node/Node.h"
#include "PingPongPubSubTypes.hpp" // 包含新生成的头文件

// 别忘了 Traits
template<>
struct MsgTypeSupportTraits<PingPongMsg> {
    using PubSubType = PingPongMsgPubSubType;
};

using namespace tiny_mw::node;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage: ./day11_latency [ping|pong]" << std::endl;
        return 0;
    }

    std::string mode = argv[1];
    Node node(mode == "ping" ? "PingNode" : "PongNode");

    if (mode == "pong") {
        // ======================= PONG (服务端) =======================
        auto pub = node.create_publisher<PingPongMsg>("PongTopic");
        
        node.create_subscription<PingPongMsg>("PingTopic", [pub](const PingPongMsg& msg) {
            // 收到 Ping，立刻发回 (Echo)
            // 零拷贝的关键：不要做多余的内存操作
            pub->publish(msg); 
        });
        
        std::cout << "[Pong] Ready to echo..." << std::endl;
        node.spin();
    } 
    else {
        // ======================= PING (客户端) =======================
        auto pub = node.create_publisher<PingPongMsg>("PingTopic");
        
        // 记录发送时间
        static auto last_send_time = std::chrono::steady_clock::now();
        
        // 订阅 Pong 回信
        node.create_subscription<PingPongMsg>("PongTopic", [](const PingPongMsg& msg) {
            auto now = std::chrono::steady_clock::now();
            // 这里其实应该用 msg.timestamp_ns 来计算，或者简单点用本地时间差
            auto diff = std::chrono::duration_cast<std::chrono::microseconds>(now - last_send_time).count();
            
            std::cout << ">>> Latency (RoundTrip/2): " << diff / 2.0 << " us" << std::endl;
            
            // 收到回复后，稍微停顿一下再发下一个，方便观察
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            
            // 发起下一次 Ping
            PingPongMsg new_msg;
            last_send_time = std::chrono::steady_clock::now();
            // 这里的 new_msg 数据其实不重要，重要的是触发
            // pub->publish(new_msg); // 这里无法在回调里直接捕获 pub 并调用，需要设计一下逻辑
            // 为了简单，我们在 Timer 里发 Ping
        });

        // 用 Timer 触发第一次 Ping (后续可以在回调里触发循环，或者 Timer 持续触发)
        auto timer = node.create_wall_timer(1000, [pub]() {
            PingPongMsg msg;
            msg.id(1);
            last_send_time = std::chrono::steady_clock::now();
            pub->publish(msg);
        });

        std::cout << "[Ping] Pinging..." << std::endl;
        node.spin();
    }

    return 0;
}