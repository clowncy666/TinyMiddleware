#include "tiny_mw/core/ParamClient.h"
#include <hiredis/hiredis.h>
#include <iostream>

namespace tiny_mw {
namespace core {

ParamClient::ParamClient() : context_(nullptr) {}

ParamClient::~ParamClient() {
    if (context_) {
        redisFree(context_);
    }
}

bool ParamClient::connect() {
    // 默认连接本地 localhost:6379
    context_ = redisConnect("127.0.0.1", 6379);
    
    if (context_ == nullptr || context_->err) {
        if (context_) {
            std::cerr << "[ParamClient] Connection error: " << context_->errstr << std::endl;
            redisFree(context_);
            context_ = nullptr;
        } else {
            std::cerr << "[ParamClient] Connection error: can't allocate redis context" << std::endl;
        }
        return false;
    }
    std::cout << "[ParamClient] Connected to Redis." << std::endl;
    return true;
}

void ParamClient::set(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!context_) return;

    // 执行 SET 命令
    // %s 是占位符，类似 printf
    redisReply* reply = (redisReply*)redisCommand(context_, "SET %s %s", key.c_str(), value.c_str());
    
    if (reply) {
        freeReplyObject(reply); // 必须释放！
    } else {
        std::cerr << "[ParamClient] SET failed" << std::endl;
    }
}

std::string ParamClient::get(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!context_) return "";

    // 执行 GET 命令
    redisReply* reply = (redisReply*)redisCommand(context_, "GET %s", key.c_str());
    
    std::string result = "";
    if (reply) {
        if (reply->type == REDIS_REPLY_STRING) {
            result = reply->str;
        }
        freeReplyObject(reply); // 必须释放！
    }
    return result;
}

} // namespace core
} // namespace tiny_mw