#pragma once
#include <string>
#include <functional>
#include <memory>
#include <vector>
#include <iostream>
#include "tiny_mw/core/EventLoop.h"
#include "tiny_mw/core/ThreadPool.h"
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>

//暂时硬编码，以后改成模板
//#include "HelloWorldPubSubTypes.hpp"
// 【新增】必须包含以下两个头文件，因为模板函数中用到了 eventfd 和 read
#include <sys/eventfd.h> // for eventfd
#include <unistd.h>      // for read, write
template<typename MsgType>
struct MsgTypeSupportTraits{};
// 前置声明 FastDDS 的类，避免把巨大的头文件暴露给用户
/*namespace eprosima {
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
}*/
namespace tiny_mw {
namespace node {
// [新增] 泛型 Publisher 包装器
template<typename MsgType>
class Publisher {
public:
    // 构造函数：持有 FastDDS 的 DataWriter
    Publisher(eprosima::fastdds::dds::DataWriter* writer) : writer_(writer) {}
    
    // 发送消息接口
    bool publish(const MsgType& msg) {
        // FastDDS 的 write 接受 void*，我们需要把 msg 的地址传进去
        // const_cast 是因为某些旧版 FastDDS API 签名问题，通常可以直接传 &msg
        return writer_->write(const_cast<void*>(static_cast<const void*>(&msg)));
    }

private:
    eprosima::fastdds::dds::DataWriter* writer_;
};
class BridgeListener : public eprosima::fastdds::dds::DataReaderListener { 
public:
    BridgeListener(int fd) : efd_(fd) {}
    void on_data_available(eprosima::fastdds::dds::DataReader* reader) override {
        uint64_t u=1;
        write(efd_,&u,sizeof(uint64_t));
    }
private:
    int efd_;
};

class Node {
public:
    //using MsgCallback =  std::function<void(const HelloWorld&)>;
    // 构造函数：初始化所有底层资源
    explicit Node(const std::string& node_name);
    ~Node();
    // 核心功能 1：创建订阅
    // 当收到消息，自动放入线程池，然后调用 callback
    //void create_subscription(const std::string& topic_name, MsgCallback callback);
    // 核心功能 2：开始运行 (阻塞主线程)
    void spin();
    // 【核心修改】泛型订阅接口
    template<typename MsgType>
    void create_subscription(const std::string& topic_name, std::function<void(const MsgType&)> callback) {
       using namespace eprosima::fastdds::dds;
       // 1. [核心魔法] 通过 Traits 找到对应的 PubSubType
       using PubSubType = typename ::MsgTypeSupportTraits<MsgType>::PubSubType;
       // 2. 准备 EventFD
        int efd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);

        // 3. 注册类型 (泛型化)
        // 创建支持类实例
        TypeSupport type_support(new PubSubType());
        // 注册到 participant (名字会自动获取，例如 "HelloWorld")
        type_support.register_type(participant_);
        std::string type_name = type_support.get_type_name();

        // 4. 创建 Topic
        // 注意：FastDDS 允许重复创建同名 Topic，但 QoS 必须一致。
        // 为了简单，我们这里直接创建，如果不检查可能会有警告，但在学习阶段没问题。
        //Topic* topic = participant_->create_topic(topic_name, type_name, TOPIC_QOS_DEFAULT);
        // 4. 【修复】获取或创建 Topic
        Topic* topic = static_cast<Topic*>(participant_->lookup_topicdescription(topic_name));
        if (topic == nullptr) {
            topic = participant_->create_topic(topic_name, type_name, TOPIC_QOS_DEFAULT);
        }
        
        // 安全检查：如果还为空，说明出大问题了，不能继续
        if (topic == nullptr) {
            std::cerr << "[Node Error] Failed to get or create topic: " << topic_name << std::endl;
            close(efd); // 记得清理资源
            return;
        }
        // 5. 创建 Listener 和 Reader
        BridgeListener* listener = new BridgeListener(efd);
        DataReader* reader = subscriber_->create_datareader(topic, DATAREADER_QOS_DEFAULT, listener);

        // 6. 保存上下文 (防止智能指针析构导致资源释放)
        // 这里需要用 void* 或者基类来存储 reader，因为我们要存进 vector
        auto ctx = std::make_shared<SubContext>();
        ctx->topic = topic;
        ctx->reader = reader;
        ctx->listener = listener;
        ctx->event_fd = efd;
        // 注意：因为是模板，我们没法知道具体的 PubSubType 指针类型，
        // 但 FastDDS 内部管理了 TypeSupport，我们不需要长期持有 PubSubType 对象
        subs_.push_back(ctx);

        // 7. 注册回调到 EventLoop
        loop_->addEvent(efd, [this, reader, callback, efd]() {
            // A. 清除信号
            uint64_t u;
            read(efd, &u, sizeof(uint64_t));

            // B. 泛型读取数据
            MsgType sample;
            SampleInfo info;
            while (reader->take_next_sample(&sample, &info) ==0) {
                if (info.valid_data) {
                    // C. 拷贝并放入线程池
                    pool_->enqueue([callback, sample]() {
                        callback(sample);
                    });
                }
            }
        });
        
        std::cout << "[Node] Subscribed to topic: " << topic_name 
                  << " [" << type_name << "]" << std::endl;
    }
    // [新增] 泛型发布接口
    template<typename MsgType>
    std::shared_ptr<Publisher<MsgType>> create_publisher(const std::string& topic_name) 
    {
        using namespace eprosima::fastdds::dds;

        // 1. [Traits] 查找对应的 PubSubType
        using PubSubType = typename ::MsgTypeSupportTraits<MsgType>::PubSubType;

        // 2. 注册类型 (如果已经注册过，FastDDS 会处理，或者我们应该检查一下)
        // 这里为了简化，每次都尝试注册。实际生产代码应该用 map 缓存 type_name
        TypeSupport type_support(new PubSubType());
        type_support.register_type(participant_);
        std::string type_name = type_support.get_type_name();

        // 3. 创建 Topic
        //Topic* topic = participant_->create_topic(topic_name, type_name, TOPIC_QOS_DEFAULT);
        // 3. 【修复】查找或创建 Topic
        // 先尝试查找是否已存在该 Topic
        Topic* topic = static_cast<Topic*>(participant_->lookup_topicdescription(topic_name));
        if (topic == nullptr) {
            // 不存在则创建
            topic = participant_->create_topic(topic_name, type_name, TOPIC_QOS_DEFAULT);
        }

        // 必须检查 topic 是否为 nullptr，否则后面 create_datawriter 必崩
        if (topic == nullptr) {
            std::cerr << "[Node Error] Failed to get or create topic: " << topic_name << std::endl;
            return nullptr;
        }
        // 4. 创建 DataWriter
        DataWriter* writer = dds_publisher_->create_datawriter(topic, DATAWRITER_QOS_DEFAULT);

        if (!writer) {
            std::cerr << "[Node] Failed to create DataWriter for " << topic_name << std::endl;
            return nullptr;
        }

        std::cout << "[Node] Created publisher for: " << topic_name << std::endl;

        // 5. 返回包装好的 Publisher 对象
        return std::make_shared<Publisher<MsgType>>(writer);
    }
private:
    std::string name_;
    std::shared_ptr<core::EventLoop> loop_;
    std::shared_ptr<core::ThreadPool> pool_;
    eprosima::fastdds::dds::DomainParticipant* participant_;
    eprosima::fastdds::dds::Subscriber* subscriber_;
    eprosima::fastdds::dds::Publisher* dds_publisher_;
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