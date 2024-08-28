#include "../galay/galay.h"
#include <thread>
#include <iostream>

galay::coroutine::NetCoroutine func()
{
    std::cout << "func start\n";
    galay::coroutine::WaitGroup group;
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
    galay::coroutine::NetCoroutinePool::GetInstance()->Stop();
    co_return galay::coroutine::kCoroutineFinished;
}

int main()
{
    spdlog::set_level(spdlog::level::debug);
    auto f = func();
    galay::coroutine::NetCoroutinePool::GetInstance()->Resume(f.GetCoId());
    std::cout << "main waiting...\n";
    galay::coroutine::NetCoroutinePool::GetInstance()->WaitForAllDone();
    std::cout << "main end...\n";
    return 0;
}