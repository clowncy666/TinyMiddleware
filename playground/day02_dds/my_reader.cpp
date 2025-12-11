#include "HelloWorldPubSubTypes.hpp"
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <iostream>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <thread>
#include <chrono>
using namespace eprosima::fastdds::dds;

class MyListener : public DataReaderListener
{
public:
    void on_data_available(DataReader* reader) override
    {
        HelloWorld data;
        SampleInfo info;
        if (reader->take_next_sample(&data, &info) == RETCODE_OK)
        {
            if (info.valid_data)
            {
                std::cout << "Received sample: index=" << data.index() << ", message='" << data.message().c_str() << "'" << std::endl;
            }
        }
    }
};
int main()
{
    // 1. 创建参与者
    DomainParticipantQos participant_qos;
    participant_qos.name("My_Subscriber_Node");
    DomainParticipant* participant = DomainParticipantFactory::get_instance()->create_participant(0, participant_qos);
    // 2. 注册数据类型
    TypeSupport type(new HelloWorldPubSubType());
    type.register_type(participant);
    // 3. 创建主题
    Topic* topic = participant->create_topic("HelloWorldTopic", type.get_type_name(), TOPIC_QOS_DEFAULT);
    // 4. 创建订阅者
    Subscriber* subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
    // 5. 创建数据读取器
    MyListener listener;
    DataReaderQos reader_qos; 
    DataReader* reader = subscriber->create_datareader(topic, reader_qos, &listener);

    std::cout << "Subscriber is running. Wait for data" << std::endl;

    while(true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(100));
    }
    return 0;
}