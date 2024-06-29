#include "../galay/galay.h"
#include <thread>
#include <iostream>

galay::kernel::GY_TcpCoroutine<galay::kernel::CoroutineStatus> func(std::thread& th)
{
    std::cout << "func start\n";
    galay::kernel::WaitGroup group;
    group.Add(1);
    th = std::thread([&](){
        std::cout << "thread start\n";
        std::cout << "thread waiting....\n";
        sleep(5);
        std::cout << "thread end\n";
        group.Done();
    });
    std::cout << "func waiting ....\n";
    co_await group.Wait();
    std::cout << "func end\n";
}

int main()
{
    std::thread th;
    auto co = func(th);
    std::cout << "main waiting....\n";
    getchar();
    if(th.joinable()) th.join();
    return 0;
}