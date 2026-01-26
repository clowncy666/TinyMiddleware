#include <iostream>
#include <thread>
#include <string>
#include <unistd.h> // for sleep

#include "tiny_mw/node/Node.h"
// 这次我们不需要 IDL，因为只测参数服务

using namespace tiny_mw::node;

int main() {
    Node node("ConfigNode");

    // 1. 设置全局参数
    std::cout << "[Main] Setting global parameters..." << std::endl;
    node.set_parameter("robot_name", "Wall-E");
    node.set_parameter("max_speed", "1.5");
    node.set_parameter("camera_resolution", "1080p");

    // 2. 模拟读取参数 (可以是另一个节点)
    std::cout << "[Main] Reading parameters from Redis..." << std::endl;
    
    std::string name = node.get_parameter("robot_name");
    std::string speed = node.get_parameter("max_speed");
    
    std::cout << ">>> Robot Name: " << name << std::endl;
    std::cout << ">>> Max Speed : " << speed << " m/s" << std::endl;

    // 3. 验证动态修改
    // 你可以在程序运行的时候，打开一个新的终端输入: 
    // redis-cli SET robot_name "Eva"
    // 然后看看这里能不能读到变化。
    
    // 这里我们用一个 Timer 模拟持续监控参数
    auto timer = node.create_wall_timer(2000, [&node]() {
        std::string current_name = node.get_parameter("robot_name");
        std::cout << "[Monitor] Current Robot Name in Redis: " << current_name << std::endl;
    });

    node.spin();
    return 0;
}