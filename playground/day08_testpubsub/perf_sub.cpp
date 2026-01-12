#include "tiny_mw/node/Node.h"
#include "HelloWorldPubSubTypes.hpp" // 确保包含你的 IDL 头文件
#include <chrono>
#include <iostream>
#include <atomic>

using namespace tiny_mw::node;
template<>
struct MsgTypeSupportTraits<HelloWorld> {
    using PubSubType = HelloWorldPubSubType;
};

std::atomic<int> g_count{0};
std::atomic<unsigned long> g_last_index{0};
std::atomic<int> g_lost_count{0};

int main() {
    auto node = std::make_shared<Node>("PerfSubNode");

    // 统计线程
    std::thread stat_thread([](){
        while(true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            int count = g_count.exchange(0);
            std::cout << "--- Stats ---" << std::endl;
            std::cout << "Rx Rate: " << count << " msgs/sec" << std::endl;
            std::cout << "Total Lost: " << g_lost_count.load() << std::endl;
        }
    });
    stat_thread.detach();

    node->create_subscription<HelloWorld>("PerfTopic", [](const HelloWorld& msg) {
        // 1. 增加计数
        g_count++;

        // 2. 简单的丢包检测 (假设 index 是连续增加的)
        unsigned long current_idx = msg.index();
        unsigned long expected = g_last_index + 1;
        
        // 如果不是第一帧，且当前index不等于上一帧+1，说明中间丢了
        if (g_last_index != 0 && current_idx > expected) {
            g_lost_count += (current_idx - expected);
        }
        g_last_index = current_idx;
        
        // 为了测试极限性能，不要在这里打印 log (std::cout 极其慢)
    });

    node->spin();
    return 0;
}