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
    std::cout << "func waiting ....\n";
    co_await group.Wait();
    pool.lock()->Stop();
    std::cout << "func end\n";
}

int main()
{
    std::shared_ptr<std::condition_variable> cond = std::make_shared<std::condition_variable>();
    galay::common::GY_NetCoroutinePool::ptr pool = std::make_shared<galay::common::GY_NetCoroutinePool>(cond);
    std::thread th([pool]{
        pool->Start();
    });
    auto f = func(pool);
    std::mutex mtx;
    std::unique_lock lock(mtx);
    cond->wait(lock);
    std::cout << "main end...\n";
    return 0;
}