#include "../galay/galay.h"
#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/async_logger.h>
#include <spdlog/async.h>

galay::thread::ThreadPool::ptr threadPool = std::make_shared<galay::thread::ThreadPool>();

galay::coroutine::NetCoroutine func(galay::poller::SelectScheduler::wptr scheduler)
{
    galay::client::HttpAsyncClient::ptr client = std::make_shared<galay::client::HttpAsyncClient>(scheduler);
    auto req = galay::protocol::http::DefaultHttpRequest();
    std::cout << req->Header()->Version() << '\n';
    auto res = client->Get("http://180.101.50.242", req);
    std::cout << "wait for response\n";
    co_await res.Wait();
    std::cout << "wait over\n";
    auto resp = res.GetResponse();
    std::cout << resp.Body() << '\n';
    galay::coroutine::NetCoroutinePool::GetInstance()->Stop();
    scheduler.lock()->Stop();
    threadPool->Stop();
    co_return galay::coroutine::kCoroutineFinished;
}


int main()
{
    spdlog::set_level(spdlog::level::info);
    galay::poller::SelectScheduler::ptr scheduler = std::make_shared<galay::poller::SelectScheduler>(50);
    threadPool->Start(1);
    threadPool->AddTask([&](){ scheduler->Start(); });
    auto co = func(scheduler);
    if(!galay::coroutine::NetCoroutinePool::GetInstance()->IsDone()) {
        std::cout << "wait for all done\n";
        galay::coroutine::NetCoroutinePool::GetInstance()->WaitForAllDone();
    }
    if(!threadPool->IsDone()) threadPool->WaitForAllDone();
    return 0;
}