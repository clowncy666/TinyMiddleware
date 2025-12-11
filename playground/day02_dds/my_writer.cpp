#include "HelloWorldPubSubTypes.hpp"
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <thread>
#include <iostream>

using namespace eprosima::fastdds::dds;

int main() {
    // 1. 创建参与者 (Participant)
    DomainParticipantQos participant_qos;
    participant_qos.name("My_Publisher_Node");
    DomainParticipant* participant = DomainParticipantFactory::get_instance()->create_participant(0, participant_qos);

    // 2. 注册数据类型
    TypeSupport type(new HelloWorldPubSubType());
    type.register_type(participant);

    // 3. 创建主题 (Topic)
    Topic* topic = participant->create_topic("HelloWorldTopic", type.get_type_name(), TOPIC_QOS_DEFAULT);

    // 4. 创建发布者 (Publisher)
    Publisher* publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT);

    // 5. 创建数据写入器 (DataWriter)
    DataWriter* writer = publisher->create_datawriter(topic, DATAWRITER_QOS_DEFAULT);
    
    std::cout << "Publisher running..." << std::endl;

    // 6. 循环发送数据
    int i = 0;
    while (true) {
        HelloWorld data;
        data.index(i++);
        data.message("Hello FastDDS");
        
        writer->write(&data);
        std::cout << "Sent: " << data.message() << " [" << data.index() << "]" << std::endl;
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}