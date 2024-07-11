#include "../galay/galay.h"
#include <thread>
#include <iostream>

galay::common::GY_NetCoroutine<galay::common::CoroutineStatus> func(galay::common::GY_NetCoroutinePool::wptr pool)
{
    std::cout << "func start\n";
    galay::kernel::WaitGroup group(pool);
    group.Add(1);
    std::thread th = std::thread([&](){
        std::cout << "thread start\n";
        std::cout << "thread waiting....\n";
        sleep(5);
        std::cout << "thread end\n";
        group.Done();
    });
    th.detach();
    std::cout << "func waiting ....\n";
    co_await group.Wait();
    std::cout << "func end\n";
    pool.lock()->Stop();
    co_return galay::common::kCoroutineFinished;
}

int main()
{
    spdlog::set_level(spdlog::level::debug);
    galay::common::GY_NetCoroutinePool::ptr pool = std::make_shared<galay::common::GY_NetCoroutinePool>();
    std::thread th([pool]{
        pool->Start();
    });
    th.detach();
    auto f = func(pool);
    uint64_t coId = f.GetCoId();
    pool->AddCoroutine(coId, std::move(f));
    std::cout << "main waiting...\n";
    pool->WaitForAllDone();
    std::cout << "main end...\n";
    return 0;
}