#include "../galay/galay.h"
#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/async_logger.h>
#include <spdlog/async.h>

galay::common::GY_ThreadPool::ptr threadPool = std::make_shared<galay::common::GY_ThreadPool>();

galay::common::GY_NetCoroutine func(galay::kernel::GY_SelectScheduler::wptr scheduler)
{
    galay::kernel::GY_SmtpAsyncClient::ptr client = std::make_shared<galay::kernel::GY_SmtpAsyncClient>(scheduler);
    client->Connect("smtp://117.135.207.210");
    auto res = client->Auth("bigdata_C1004@163.com", "EPOXVZMINXCXHXUO");
    co_await res->Wait();
    if(!res->Success()) std::cout << res->Error() << '\n';
    galay::protocol::smtp::SmtpMsgInfo msg;
    msg.m_subject = "test";
    msg.m_content = "<html><head>Verify</head><body>111111</body></html>";
    std::vector<std::string> arr = {"gzj17607270093@163.com", "1790188287@qq.com"};
    res = client->SendEmail("bigdata_C1004@163.com", arr, msg);
    co_await res->Wait();
    if(!res->Success()) std::cout << res->Error() << '\n';
    res = client->Quit();
    co_await res->Wait();
    if(!res->Success()) std::cout << res->Error() << '\n';
    
    galay::common::GY_NetCoroutinePool::GetInstance()->Stop();
    scheduler.lock()->Stop();
    threadPool->Stop();
    co_return galay::common::kCoroutineFinished;
}


int main()
{
    spdlog::set_level(spdlog::level::trace);
    galay::kernel::GY_SelectScheduler::ptr scheduler = std::make_shared<galay::kernel::GY_SelectScheduler>(50);
    threadPool->Start(1);
    galay::common::GY_NetCoroutinePool::GetInstance()->Start();
    threadPool->AddTask([&](){ scheduler->Start(); });
    auto co = func(scheduler);
    if(!galay::common::GY_NetCoroutinePool::GetInstance()->IsDone()) galay::common::GY_NetCoroutinePool::GetInstance()->WaitForAllDone();
    if(!threadPool->IsDone()) threadPool->WaitForAllDone();
    return 0;
}