#include <iostream>
#include <thread>
#include <vector>
#include <chrono>

#include "tiny_mw/node/Node.h"

// 1. 包含 Carrier (快递箱) - 之前生成的 FastDDS 代码
#include "BlobMsgPubSubTypes.hpp"

// 2. 包含生成的 Protobuf 头文件 (CMake 会自动生成这个文件)
#include "robot.pb.h" 

using namespace tiny_mw::node;

// =============================================================
// 特化 Traits：告诉 Node，BlobMsg 对应 BlobMsgPubSubType
// 我们只需要注册这一个类型，就可以发所有 Proto 消息！
// =============================================================
template<>
struct MsgTypeSupportTraits<BlobMsg> {
    using PubSubType = BlobMsgPubSubType;
};

// =============================================================
// 辅助函数：序列化与反序列化工具
// =============================================================
// 将 Protobuf 序列化为 BlobMsg
/*BlobMsg to_blob(const pb::RobotStatus& proto_msg) {
    BlobMsg blob;
    blob.type_name("pb.RobotStatus"); // 标记类型，方便接收端识别
    
    // Protobuf -> string (二进制流)
    std::string serialized_data;
    proto_msg.SerializeToString(&serialized_data);
    
    // string -> vector<char> (DDS sequence)
    // 注意：这里涉及一次内存拷贝。生产环境可以用 ZeroCopy 优化，但学习阶段先跑通逻辑。
    std::vector<char> vec(serialized_data.begin(), serialized_data.end());
    blob.payload(vec);
    
    return blob;
}

// 将 BlobMsg 反序列化为 Protobuf
pb::RobotStatus from_blob(const BlobMsg& blob) {
    pb::RobotStatus proto_msg;
    // vector<char> -> string -> Protobuf
    // 同样涉及拷贝，反序列化
    std::string str(blob.payload().begin(), blob.payload().end());
    proto_msg.ParseFromString(str);
    return proto_msg;
}*/
BlobMsg to_blob_optimized(const pb::RobotStatus& proto_msg) {
    BlobMsg blob;
    blob.type_name("pb.RobotStatus");

    // 1. 预先询问 Protobuf：你要占多大地方？
    // ByteSizeLong() 是 Protobuf 提供的快速计算大小的方法
    size_t data_size = proto_msg.ByteSizeLong();

    // 2. 准备容器：直接把箱子(vector)撑大到指定尺寸
    // resize 会直接分配好内存，并且保证是连续的
    blob.payload().resize(data_size);

    // 3. 直接写入：跳过 string，直接写进 vector 的内存首地址
    // data() 返回 vector 内部数组的指针
    // 这一步是“必要的序列化拷贝”，但这已经是唯一的一次拷贝了！
    proto_msg.SerializeToArray(blob.payload().data(), data_size);

    return blob;
}
pb::RobotStatus from_blob_optimized(const BlobMsg& blob) {
    pb::RobotStatus proto_msg;

    // 直接告诉 Protobuf：数据在这里，长度是这么多，你自己去读
    // 不需要先转成 string，Protobuf 支持直接解析 raw memory
    const void* data_ptr = blob.payload().data();
    int data_len = static_cast<int>(blob.payload().size());

    // 解析数组
    if (!proto_msg.ParseFromArray(data_ptr, data_len)) {
        std::cerr << "Failed to parse Protobuf message!" << std::endl;
        // 在实际项目中这里应该抛异常或返回错误码
    }

    return proto_msg;
}

int main() {
    Node node("ProtoNode");

    // 1. 创建基于 BlobMsg 的发布者
    // 注意：我们在 DDS 层只传输 BlobMsg，完全不依赖 RobotStatus 的 IDL
    auto pub = node.create_publisher<BlobMsg>("GlobalTopic");

    // 2. 发送线程
    std::thread t([pub]() {
        int count = 0;
        while(true) {
            // A. 准备业务数据 (Protobuf 对象)
            pb::RobotStatus status;
            status.set_id(++count);
            status.set_state(count % 2 == 0 ? "IDLE" : "MOVING");
            status.set_battery_level(95.5f - count * 0.1f);
            status.add_position(1.0f);
            status.add_position(2.0f);
            status.add_position(3.0f);

            // B. 封装进快递箱 (Serialize)
            BlobMsg blob = to_blob_optimized(status);

            // C. 发送快递箱
            pub->publish(blob);

            std::cout << ">>> [Proto Pub] ID: " << status.id() 
                      << " | State: " << status.state()
                      << " | Blob Size: " << blob.payload().size() << " bytes" << std::endl;

            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    });

    // 3. 订阅者
    node.create_subscription<BlobMsg>("GlobalTopic", [](const BlobMsg& blob) {
        // A. 检查类型 (这是一个好习惯，虽然这里只有一个类型)
        if (blob.type_name() != "pb.RobotStatus") {
            std::cout << "Unknown message type: " << blob.type_name() << std::endl;
            return;
        }

        // B. 开箱 (Deserialize)
        pb::RobotStatus status = from_blob_optimized(blob);

        std::cout << "<<< [Proto Sub] ID: " << status.id() 
                  << " | Bat: " << status.battery_level() 
                  << " | Pos: [" << status.position(0) << ", " << status.position(1) << "]" << std::endl;
    });
    node.spin();
    t.join();
    return 0;
}