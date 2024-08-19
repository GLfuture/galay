#include "../galay/galay.h"
#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/async_logger.h>
#include <spdlog/async.h>

galay::thread::GY_ThreadPool::ptr threadPool = std::make_shared<galay::thread::GY_ThreadPool>();

galay::coroutine::GY_NetCoroutine func(galay::poller::GY_SelectScheduler::wptr scheduler)
{
    galay::client::GY_HttpAsyncClient::ptr client = std::make_shared<galay::client::GY_HttpAsyncClient>(scheduler);
    auto req = galay::protocol::http::DefaultHttpRequest();
    std::cout << req->Header()->Version() << '\n';
    auto res = client->Get("https://180.101.50.242", req);
    co_await res->Wait();
    auto resp = res->GetResponse();
    std::cout << resp.Body() << '\n';
    galay::coroutine::GY_NetCoroutinePool::GetInstance()->Stop();
    scheduler.lock()->Stop();
    threadPool->Stop();
    co_return galay::coroutine::kCoroutineFinished;
}


int main()
{
    spdlog::set_level(spdlog::level::debug);
    galay::poller::GY_SelectScheduler::ptr scheduler = std::make_shared<galay::poller::GY_SelectScheduler>(50);
    threadPool->Start(1);
    galay::coroutine::GY_NetCoroutinePool::GetInstance()->Start();
    threadPool->AddTask([&](){ scheduler->Start(); });
    auto co = func(scheduler);
    if(!galay::coroutine::GY_NetCoroutinePool::GetInstance()->IsDone()) galay::coroutine::GY_NetCoroutinePool::GetInstance()->WaitForAllDone();
    if(!threadPool->IsDone()) threadPool->WaitForAllDone();
    return 0;
}