#include "tiny_mw/node/Node.h"
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