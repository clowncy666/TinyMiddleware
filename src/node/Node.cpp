#include "tiny_mw/node/Node.h"
#include <iostream>
#include <sys/eventfd.h>
#include <unistd.h>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <sys/timerfd.h>
#include <cstring> // for memset
#include "tiny_mw/core/ParamClient.h"

using namespace eprosima::fastdds::dds;
namespace tiny_mw {
namespace node {

Node::Node(const std::string& node_name) : name_(node_name) {
    //1.初始化引擎
    loop_=std::make_shared<core::EventLoop>();
    pool_=std::make_shared<core::ThreadPool>(4);
    // 初始化参数客户端
    param_client_ = std::make_unique<core::ParamClient>();
    // 尝试连接，如果不成功也别崩，打印个警告就行
    if (!param_client_->connect()) {
        std::cerr << "[Node Warning] Parameter Server (Redis) not available!" << std::endl;
    }
    // 尝试加载根目录下的 shm_profile.xml
    // 如果文件不存在，FastDDS 会报错但程序会继续（回退到默认）


    DomainParticipantFactory::get_instance()->load_XML_profiles_file("shm_profile.xml");
    participant_ = DomainParticipantFactory::get_instance()->create_participant_with_profile(
        0, "shm_participant_profile");

    // 如果 XML 加载失败 (返回 nullptr)，则使用代码中的默认 QoS
    if (participant_ == nullptr) {
        std::cout << "[Node] SHM profile not found, falling back to DEFAULT QoS..." << std::endl;
        
        DomainParticipantQos pqos;
        pqos.name(node_name);
        participant_ = DomainParticipantFactory::get_instance()->create_participant(0, pqos);
    } else {
        std::cout << "[Node] Loaded High-Performance SHM Profile!" << std::endl;
    }
    // -------------------------------------------------------------------------

    // 最后安全检查：如果还是空的，说明 FastDDS 彻底挂了
    if (participant_ == nullptr) {
        std::cerr << "[NODE Error] Failed to create participant!" << std::endl;
        exit(-1);
    }
    subscriber_=participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
    dds_publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT);
    if (!subscriber_ || !dds_publisher_) {
        std::cerr << "[NODE Error] Failed to create subscriber or publisher!" << std::endl;
        exit(-1);
    }
}
Node::~Node() {
    //先loop停止，再pool停止,再释放DDS
    loop_->stop();
}
void Node::spin() {
    std::cout<<"[NODE] "<<name_<<" start spinning"<<std::endl;
    loop_->run();
}
/*void Node::create_subscription(const std::string& topic_name, MsgCallback callback) {
    //1.创建eventfd
    int efd=eventfd(0,EFD_NONBLOCK | EFD_CLOEXEC);
    if(efd==-1) {
        std::cerr<<"[NODE] fail to create eventfd"<<std::endl;
        return;
    }
    //2.dds资源准备
    // 注意：这里需要检查 Topic 是否已存在，为简单起见直接创建
    Topic* topic = participant_->create_topic(topic_name, "HelloWorld", TOPIC_QOS_DEFAULT);
    
    BridgeListener* listener = new BridgeListener(efd);
    DataReader* reader = subscriber_->create_datareader(topic, DATAREADER_QOS_DEFAULT, listener);
    //3.保存上下文防止内存泄漏
    auto ctx=std::make_shared<SubContext>();
    ctx->topic = topic;
    ctx->reader = reader;
    ctx->listener = listener;
    ctx->event_fd = efd;
    subs_.push_back(ctx);
    //4.eventloop绑定
    loop_->addEvent(efd, [this, reader, callback, efd]() {
        // A. 清除信号
        uint64_t u;
        read(efd, &u, sizeof(uint64_t));

        // B. 拿数据
        HelloWorld sample;
        SampleInfo info;
        while (reader->take_next_sample(&sample, &info) == 0) {
            if (info.valid_data) {
                // C. 丢进线程池 (拷贝 sample)
                pool_->enqueue([callback, sample]() {
                    // D. 执行用户逻辑
                    callback(sample);
                });
            }
        }
    });*/

std::shared_ptr<TimerContext> Node::create_wall_timer(
    int period_ms, 
    std::function<void()> callback) {
    // 1. 创建 timerfd
    //CLOCK_MONOTONIC: 系统启动后流逝的时间，不受修改系统时间影响（稳！）
    int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (tfd == -1) {
        std::cerr << "[Node] Failed to create timerfd: " <<std::endl;
        return nullptr;
    }
    // 2. 设置时间
    struct itimerspec new_value;
    memset(&new_value, 0, sizeof(new_value));
    // 首次触发时间 (立刻触发还是等一个周期？这里设置等一个周期)
    new_value.it_value.tv_sec = period_ms / 1000;
    new_value.it_value.tv_nsec = (period_ms % 1000) * 1000000;

    // 周期时间 (Interval)
    new_value.it_interval.tv_sec = period_ms / 1000;
    new_value.it_interval.tv_nsec = (period_ms % 1000) * 1000000;

    // 启动定时器
    if (timerfd_settime(tfd, 0, &new_value, nullptr) == -1) {
        std::cerr << "[Node] Failed to set timer time!" << std::endl;
        close(tfd);
        return nullptr;
    }
    // 3. 注册到 EventLoop
    // 这里的逻辑和 Subscriber 一模一样：Epoll 唤醒 -> 读 fd -> 丢进线程池
    loop_->addEvent(tfd, [this, tfd, callback]() {
        // A. 必须读取 timerfd，否则 epoll 会一直触发 (Level Trigger)
        // read 返回的是超时次数 (uint64_t)
        uint64_t exp;
        ssize_t s = read(tfd, &exp, sizeof(uint64_t));
        if (s != sizeof(uint64_t)) return;

        // B. 丢进线程池执行用户回调
        pool_->enqueue([callback]() {
            callback();
        });
    });

    // 4. 保存上下文并返回
    auto ctx = std::make_shared<TimerContext>();
    ctx->timer_fd = tfd;
    ctx->callback = callback;
    timers_.push_back(ctx);

    std::cout << "[Node] Created timer with period: " << period_ms << "ms" << std::endl;
    return ctx;
}
void Node::set_parameter(const std::string& key, const std::string& value) {
    // 自动给 key 加上节点名命名空间，防止冲突? 
    // 或者做成全局的。这里先做成全局的，方便大家共享。
    param_client_->set(key, value);
}

std::string Node::get_parameter(const std::string& key) {
    return param_client_->get(key);
}


} //namespace node
} //namespace tiny_mw
