#include <iostream>
#include <string>
#include "galay/middleware/Redis.h"

using namespace galay::redis;

int main() {
    std::string url = "redis://123456@127.0.0.1:6379";
    auto config = RedisConfig::CreateConfig();
    config->ConnectWithTimeout(1000);
    RedisSession session(config);
    if( !session.Connect(url) ) {
        std::cout << "connect failed" << std::endl;
        return -1;
    }
    session.Set("hello", "world");
    std::cout << session.Get("hello").ToString() << std::endl;
    session.DisConnect();
    return 0;
}