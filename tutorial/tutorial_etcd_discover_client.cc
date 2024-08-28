#include "../galay/middleware/etcd.h"
#include "../galay/galay.h"

galay::coroutine::NetCoroutine func()
{
    galay::middleware::etcd::EtcdClient client("http://127.0.0.1:2379");
    auto res = client.DiscoverServicePrefix("/app/api/login");
    co_await res.Wait();
    auto addr = res.ServiceAddrs();
    for(auto &item : addr)
    {
        std::cout << item.first << ":" << item.second << std::endl;
    }
    client.Watch("/app/api/login", [](::etcd::Response res){
        std::cout << res.value().as_string() << std::endl;
    });
    getchar();
    galay::coroutine::NetCoroutinePool::GetInstance()->Stop();
    co_return galay::coroutine::CoroutineStatus::kCoroutineFinished;
}


int main()
{
    galay::coroutine::NetCoroutine task = func();
    getchar();
    if(!galay::coroutine::NetCoroutinePool::GetInstance()->IsDone()) 
    {
        galay::coroutine::NetCoroutinePool::GetInstance()->WaitForAllDone();
    }
    return 0;
}