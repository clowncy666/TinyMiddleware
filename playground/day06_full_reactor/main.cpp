#include "tiny_mw/core/EventLoop.h"
#include "tiny_mw/core/ThreadPool.h"
#include <sys/eventfd.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <thread>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include "HelloWorldPubSubTypes.hpp"
#include <fastdds/dds/core/ReturnCode.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/topic/Topic.hpp>
using namespace eprosima::fastdds::dds;
using namespace tiny_mw::core;
class NotifyListener : public DataReaderListener
{
public:
    NotifyListener(int fd):efd_(fd){}
    void on_data_available(DataReader* reader) override
    {
        uint64_t u = 1;
        write(efd_, &u, sizeof(uint64_t));
    }
private:
    int efd_;
};




int main(){
    int efd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    EventLoop loop;
    ThreadPool pool(4);
    std::cout<<"[Main] System initialized. Main Thread ID:"<<std::this_thread::get_id()<<std::endl;
    // DDS Initialization
    TypeSupport type(new HelloWorldPubSubType());
    DomainParticipantQos pqos;
    pqos.name("Reactor_Participant");
    DomainParticipant* participant = DomainParticipantFactory::get_instance()->create_participant(0,pqos);
    type.register_type(participant);
    Topic* topic = participant->create_topic("HelloWorldTopic",type.get_type_name(),TOPIC_QOS_DEFAULT);
    Subscriber* subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
    NotifyListener* listener = new NotifyListener(efd);
    DataReader* reader = subscriber->create_datareader(topic,DATAREADER_QOS_DEFAULT,listener);
    //Work Core
    loop.addEvent(efd,[&](){
        uint64_t u;
        read(efd, &u, sizeof(uint64_t));
        HelloWorld msg;
        SampleInfo info;
        while(reader->take_next_sample(&msg, &info) == 0){
            if(info.valid_data){
                pool.enqueue([msg](){
                    std::thread::id worker_id = std::this_thread::get_id();
                    
                    // 模拟耗时业务逻辑 (比如图像解码、SLAM计算)
                    std::cout << "  [Worker " << worker_id << "] Processing: " 
                              << msg.message() << " (index: " << msg.index() << ")" << std::endl;
                    
                    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 假装在忙
                });
            }
        }
    });
    std::cout<<"[Main] Reactor loop starting..."<<std::endl;
    loop.run();
    return 0;
    
    
}