#pragma once
#include <string>
#include <memory>
#include <mutex>
struct redisContext;
namespace tiny_mw {
namespace core {
class ParamClient {
public:
    ParamClient();
    ~ParamClient();
    bool connect();
    void set(const std::string& key, const std::string& value);
    std::string get(const std::string& key);
private:
    redisContext* context_;
    std::mutex mutex_;
};
} // namespace core
} // namespace tiny_mw