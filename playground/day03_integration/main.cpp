#include <iostream>
#include <thread>
#include <vector>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/eventfd.h>
//FastDDS头文件
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/core/ReturnCode.hpp>
#include <fastdds/dds/core/ReturnCode.hpp> 
//IDL头文件
#include "HelloWorldPubSubTypes.hpp"
#include "HelloWorld.hpp"
using namespace eprosima::fastdds::dds;
//using namespace eprosima::fastrtps::types;
class BridgeListener : public DataReaderListener
{
public:
    //构造函数传入文件句柄
    BridgeListener(int fd) : fd_(fd) {}
    //DDS收到数据时，这个函数会在内部线程被调用
    void on_data_available(DataReader* reader) override
    {
        uint64_t u = 1;
        ssize_t s = write(fd_, &u, sizeof(uint64_t));
        (void)s; // 消除未使用变量的警告
        
        
    }
private:
    int fd_;
};
int main()
{
    // 1. 创建 EventFD 
    int efd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    
    // 2. 创建 Epoll 
    int epoll_fd = epoll_create1(0);
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = efd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, efd, &ev);
    //3. 初始化 Fast DDS 实体
    // 加载类型
    TypeSupport type(new HelloWorldPubSubType());
    std::string type_name = type.get_type_name();

    // 创建参与者
    DomainParticipantQos pqos;
    pqos.name("Integration_Participant");
    DomainParticipant* participant = DomainParticipantFactory::get_instance()->create_participant(0, pqos);
    
    // 注册类型
    type.register_type(participant);

    // 创建 Topic
    Topic* topic = participant->create_topic("HelloWorldTopic", type_name, TOPIC_QOS_DEFAULT);

    // 创建 Subscriber
    Subscriber* subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT);

    // 创建 Listener (关键！把 eventfd 传进去)
    BridgeListener* listener = new BridgeListener(efd);

    // 创建 DataReader
    DataReader* reader = subscriber->create_datareader(topic, DATAREADER_QOS_DEFAULT, listener);
    std::cout << "[Main] Waiting for DDS messages via Epoll..." << std::endl;
    // 4. 进入主循环 (Reactor Loop)
    const int MAX_EVENTS = 10;
    struct epoll_event events[MAX_EVENTS];

    bool running = true;
    while (running) {
        // 阻塞等待
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

        for (int i = 0; i < n; ++i) {
            if (events[i].data.fd == efd) {
                // A. 听到门铃了，先去把门铃关掉 (读取 eventfd)
                uint64_t u;
                read(efd, &u, sizeof(uint64_t));

                // B. 现在我们在主线程了！可以安全地去拿信了
                // 循环读取，直到读空缓冲区（因为可能一次来了好几条）
                HelloWorld sample;
                SampleInfo info;
                while (reader->take_next_sample(&sample, &info) == static_cast<ReturnCode_t>(0)) {
                    if (info.valid_data) {
                        std::cout << "[Main-Thread] Recv: " << sample.message() 
                                  << " | Index: " << sample.index() << std::endl;
                    }
                }
            }
        }
    }

   
    return 0;
}