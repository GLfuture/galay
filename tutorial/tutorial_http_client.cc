#include "../galay/galay.h"
#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/async_logger.h>
#include <spdlog/async.h>

galay::kernel::GY_HttpAsyncClient client;
galay::kernel::GY_TcpCoroutine<galay::kernel::CoroutineStatus> func()
{
    auto resp = co_await client.Get("http://183.2.172.185:80");
    std::cout << resp->EncodePdu();
    co_return galay::kernel::CoroutineStatus::kCoroutineFinished;
}

int main()
{
    //遇到debug级别日志立即刷新
    spdlog::flush_on(spdlog::level::debug);
    spdlog::init_thread_pool(8192, 1);
    auto rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("client.log", 1024*1024*10, 3);
    std::vector<spdlog::sink_ptr> sinks {rotating_sink};
    auto logger = std::make_shared<spdlog::async_logger>("logger", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
    logger->set_level(spdlog::level::debug);
    spdlog::set_default_logger(logger);
    auto co = func();
    getchar();
    std::cout << "main" << std::endl;
    return 0;
}
