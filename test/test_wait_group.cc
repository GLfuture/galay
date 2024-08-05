#include "../galay/galay.h"
#include <thread>
#include <iostream>

galay::common::GY_NetCoroutine func()
{
    std::cout << "func start\n";
    galay::common::WaitGroup group;
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
    galay::common::GY_NetCoroutinePool::GetInstance()->Stop();
    co_return galay::common::kCoroutineFinished;
}

int main()
{
    spdlog::set_level(spdlog::level::debug);
    auto f = func();
    galay::common::GY_NetCoroutinePool::GetInstance()->Resume(f.GetCoId());
    std::cout << "main waiting...\n";
    galay::common::GY_NetCoroutinePool::GetInstance()->WaitForAllDone();
    std::cout << "main end...\n";
    return 0;
}