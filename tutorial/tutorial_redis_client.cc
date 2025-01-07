#include <iostream>
#include <string>
#include "galay/galay.hpp"
#include "galay/middleware/Redis.hpp"

using namespace galay::redis;

#define TEST_ASYNC

#ifdef TEST_ASYNC
galay::Coroutine<void> test(galay::RoutineCtx ctx)
{
    auto config = RedisConfig::CreateConfig();
    galay::redis::RedisAsyncSession session(config);
    galay::THost host{ "124.70.0.139", 6379 };
    bool res = co_await session.AsyncConnect<void>(host);
    if (!res) {
        std::cout << "connect failed: "  << std::endl;
        co_return;
    } 
    std::cout << "connect success" << std::endl;
    RedisAsyncValue value = co_await session.AsyncCommand<void>("AUTH 123456");
    std::cout << "set key value: " << value.ToString() << std::endl;
    co_return;
}


#elif defined(TEST_SYNC)
void test()
{
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
}
#endif


int main() 
{
    galay::GalayEnv env({});
#ifdef TEST_ASYNC
    test(galay::RoutineCtx::Create());
    getchar();
#elif defined(TEST_SYNC)
    test();
#endif
    return 0;
}