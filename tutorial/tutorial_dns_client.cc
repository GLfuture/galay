#include "../galay/galay.h"
#include <iostream>
#include <spdlog/spdlog.h>

galay::common::GY_ThreadPool::ptr threadPool = std::make_shared<galay::common::GY_ThreadPool>();

galay::coroutine::GY_Coroutine func(galay::kernel::GY_SelectScheduler::wptr scheduler)
{
    galay::kernel::DnsAsyncClient client(scheduler);
    auto res = client.QueryA("114.114.114.114", 53, "www.baidu.com");
    co_await res->Wait();
    while(res->HasCName()) {
        std::cout << "cname: " << res->GetCName() << std::endl;
    }
    while (res->HasA())
    {
        std::cout << "ipv4: " << res->GetA() << std::endl;
    }
    res = client.QueryAAAA("114.114.114.114", 53, "www.baidu.com");
    co_await res->Wait();
    while (res->HasAAAA())
    {
        std::cout << "ipv6: " << res->GetAAAA() << std::endl;
    }
    res = client.QueryPtr("114.114.114.114", 53, "127.0.0.1");
    co_await res->Wait();
    while (res->HasPtr())
    {
        std::cout << "ptr: " << res->GetPtr() << std::endl;
    }
    galay::coroutine::GY_NetCoroutinePool::GetInstance()->Stop();
    scheduler.lock()->Stop();
    threadPool->Stop();
    co_return galay::coroutine::kCoroutineFinished;
}

int main()
{
    spdlog::set_level(spdlog::level::debug);
    galay::kernel::GY_SelectScheduler::ptr scheduler = std::make_shared<galay::kernel::GY_SelectScheduler>(50);
    threadPool->Start(1);
    galay::coroutine::GY_NetCoroutinePool::GetInstance()->Start();
    threadPool->AddTask([&](){ scheduler->Start(); });
    auto co = func(scheduler);
    if(!galay::coroutine::GY_NetCoroutinePool::GetInstance()->IsDone()) galay::coroutine::GY_NetCoroutinePool::GetInstance()->WaitForAllDone();
    if(!threadPool->IsDone()) threadPool->WaitForAllDone();
    return 0;
}
