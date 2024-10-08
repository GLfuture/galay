#include "../galay/galay.h"
#include <iostream>
#include <spdlog/spdlog.h>

galay::thread::ThreadPool::ptr threadPool = std::make_shared<galay::thread::ThreadPool>();

galay::coroutine::Coroutine func(galay::poller::SelectScheduler::wptr scheduler)
{
    galay::client::DnsAsyncClient client(scheduler);
    auto res = client.QueryA("114.114.114.114", 53, "www.baidu.com");
    co_await res.Wait();
    while(res.HasCName()) {
        std::cout << "cname: " << res.GetCName() << std::endl;
    }
    while (res.HasA())
    {
        std::cout << "ipv4: " << res.GetA() << std::endl;
    }
    res = client.QueryAAAA("114.114.114.114", 53, "www.baidu.com");
    co_await res.Wait();
    while (res.HasAAAA())
    {
        std::cout << "ipv6: " << res.GetAAAA() << std::endl;
    }
    res = client.QueryPtr("114.114.114.114", 53, "127.0.0.1");
    co_await res.Wait();
    while (res.HasPtr())
    {
        std::cout << "ptr: " << res.GetPtr() << std::endl;
    }
    galay::coroutine::NetCoroutinePool::GetInstance()->Stop();
    scheduler.lock()->Stop();
    threadPool->Stop();
    co_return galay::coroutine::kCoroutineFinished;
}

int main()
{
    spdlog::set_level(spdlog::level::debug);
    galay::poller::SelectScheduler::ptr scheduler = std::make_shared<galay::poller::SelectScheduler>(50);
    threadPool->Start(1);
    threadPool->AddTask([&](){ scheduler->Start(); });
    auto co = func(scheduler);
    if(!galay::coroutine::NetCoroutinePool::GetInstance()->IsDone()) galay::coroutine::NetCoroutinePool::GetInstance()->WaitForAllDone();
    if(!threadPool->IsDone()) threadPool->WaitForAllDone();
    return 0;
}
