#include "../galay/galay.h"
#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/async_logger.h>
#include <spdlog/async.h>

galay::common::GY_NetCoroutine<galay::common::CoroutineStatus> func(galay::common::GY_NetCoroutinePool::wptr pool, galay::kernel::GY_SelectScheduler::wptr scheduler)
{
    // galay::kernel::GY_TcpClient::ptr client = std::make_shared<galay::kernel::GY_TcpClient>(pool, scheduler);
    // client->Socket();
    // auto result = client->Connect("127.0.0.1", 8899);
    // co_await result->Wait();
    // if(!result->Success()) std::cout << result->Error() << std::endl;
    // result = client->Send("GET /echo HTTP/1.1\r\n\r\n");
    // co_await result->Wait();
    // if(!result->Success()) std::cout << result->Error() << std::endl;
    // std::string res;
    // result = client->Recv(res);
    // co_await result->Wait();
    // if(!result->Success()) std::cout << result->Error() << std::endl;
    // else std::cout << res << std::endl;
    // client->Close();
    // pool.lock()->Stop();
    // scheduler.lock()->Stop();
    // co_return galay::common::kCoroutineFinished;
    co_await std::suspend_always{};
    galay::kernel::GY_HttpAsyncClient::ptr client = std::make_shared<galay::kernel::GY_HttpAsyncClient>(pool, scheduler);
    auto req = galay::protocol::http::DefaultHttpRequest();
    std::cout << req->Header()->Version() << '\n';
    auto res = client->Get("http://180.101.50.242", req);
    co_await res->Wait();
    std::cout << res->GetResponse()->Body() << '\n';
    co_return galay::common::kCoroutineFinished;
}


int main()
{
    spdlog::set_level(spdlog::level::debug);
    galay::common::GY_NetCoroutinePool::ptr pool = std::make_shared<galay::common::GY_NetCoroutinePool>();
    pool->Start();
    galay::kernel::GY_SelectScheduler::ptr scheduler = std::make_shared<galay::kernel::GY_SelectScheduler>(50);
    galay::common::GY_ThreadPool::ptr threadPool = std::make_shared<galay::common::GY_ThreadPool>();
    threadPool->Start(1);
    threadPool->AddTask([&](){ scheduler->Start(); });
    auto co = func(pool, scheduler);
    uint64_t coId = co.GetCoId();
    pool->AddCoroutine(std::move(co));
    pool->Resume(coId);
    getchar();
    pool->WaitForAllDone(5000);
    threadPool->WaitForAllDone(5000);
    return 0;
}