#pragma once
#include <string>
#include <functional>
#include <memory>
#include <vector>
#include "tiny_mw/core/EventLoop.h"
#include "tiny_mw/core/ThreadPool.h"
//暂时硬编码，以后改成模板
#include "HelloWorldPubSubTypes.hpp"
// 前置声明 FastDDS 的类，避免把巨大的头文件暴露给用户
namespace eprosima {
namespace fastdds {
namespace dds {
    class DomainParticipant;
    class Publisher;
    class Subscriber;
    class Topic;
    class DataReader;
    class DataReaderListener;
}
}
}
namespace tiny_mw {
namespace node {
class Node {
public:
    using MsgCallback =  std::function<void(const HelloWorld&)>;
    // 构造函数：初始化所有底层资源
    explicit Node(const std::string& node_name);
    ~Node();
    // 核心功能 1：创建订阅
    // 当收到消息，自动放入线程池，然后调用 callback
    void create_subscription(const std::string& topic_name, MsgCallback callback);
    // 核心功能 2：开始运行 (阻塞主线程)
    void spin();
private:
    std::string name_;
    std::shared_ptr<core::EventLoop> loop_;
    std::shared_ptr<core::ThreadPool> pool_;
    eprosima::fastdds::dds::DomainParticipant* participant_;
    eprosima::fastdds::dds::Subscriber* subscriber_;
    // 为了防止资源被释放，我们需要持有这些指针
    struct SubContext {
        eprosima::fastdds::dds::Topic* topic;
        eprosima::fastdds::dds::DataReader* reader;
        eprosima::fastdds::dds::DataReaderListener* listener;
        int event_fd; // 每个订阅者对应一个 eventfd
    };
    std::vector<std::shared_ptr<SubContext>> subs_;
};
} // namespace node
} // namespace tiny_mw