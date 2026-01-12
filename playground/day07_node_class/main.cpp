/*#include "tiny_mw/node/Node.h"
#include <iostream>
#include <thread>
using namespace tiny_mw::node;
int main(){
    //1.创建节点
    std::cout<<"[user] Creating node"<<std::endl;
    Node my_node("MyUserNode");
    //2.创建订阅(Lamda回调)
    my_node.create_subscription("HelloWorldTopic",
        [](const HelloWorld& msg){
            std::cout<<"[user] Received message: "<<msg.message()<<" on thread "<<std::this_thread::get_id()<<std::endl;
        }
    );
    //3.开始spin
    my_node.spin();
    return 0;
}
    */
#include <iostream>
#include <thread>
#include <string>
#include "tiny_mw/node/Node.h"

//  包含生成的 IDL 头文件 
#include "HelloWorldPubSubTypes.hpp" 
// 【关键】特化 Traits 翻译字典

template<>
struct MsgTypeSupportTraits<HelloWorld> {
    // 告诉 Node: HelloWorld 对应的支持类是 HelloWorldPubSubType
    using PubSubType = HelloWorldPubSubType;
};

using namespace tiny_mw::node;

int main() {
    std::cout << "[User] Creating Node..." << std::endl;
    Node my_node("GenericNode");

    // 4. 创建订阅：现在是泛型的了！
    // 注意尖括号 <HelloWorld>
    my_node.create_subscription<HelloWorld>("HelloWorldTopic", [](const HelloWorld& msg) {
        
        std::cout << ">>> [Recv] Index: " << msg.index() 
                  << " | Msg: " << msg.message() 
                  << " (Thread: " << std::this_thread::get_id() << ")" << std::endl;
    });

    std::cout << "[User] Start spinning..." << std::endl;
    my_node.spin();

    return 0;
}