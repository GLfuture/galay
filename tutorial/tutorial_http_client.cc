#include "../galay/galay.h"
#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/async_logger.h>
#include <spdlog/async.h>
galay::GY_TcpCoroutine<galay::CoroutineStatus> func()
{
    galay::GY_HttpAsyncClient client("http://183.2.172.185:80");
    galay::protocol::http::Http1_1_Request::ptr request = std::make_shared<galay::protocol::http::Http1_1_Request>();
    request->SetMethod("GET");
    request->SetUri("/");
    request->SetVersion("1.1");
    request->SetHeadPair({"Connection","Upgrade, HTTP2-Settings"});
    request->SetHeadPair({"Upgrade","h2c"});
    //std::string http2Settings = galay::security::Base64Util::base64_encode("")
    auto resp = co_await client.Get(request);
    std::cout << resp->EncodePdu();
    co_return galay::CoroutineStatus::kCoroutineFinished;
}

int main()
{
    //遇到debug级别日志立即刷新
    spdlog::flush_on(spdlog::level::debug);
    spdlog::init_thread_pool(8192, 1);
    auto rotating_sink = ::std::make_shared<spdlog::sinks::rotating_file_sink_mt>("client.log", 1024*1024*10, 3);
    ::std::vector<spdlog::sink_ptr> sinks {rotating_sink};
    auto logger = ::std::make_shared<spdlog::async_logger>("logger", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
    logger->set_level(spdlog::level::debug);
    spdlog::set_default_logger(logger);
    auto co = func();
    getchar();
    std::cout << "main" << std::endl;
    return 0;
}
