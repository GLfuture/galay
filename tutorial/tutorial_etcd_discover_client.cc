#include "../galay/middleware/AsyncEtcd.h"
#include "../galay/galay.h"

galay::common::GY_NetCoroutine<galay::common::CoroutineStatus> func()
{
    galay::MiddleWare::AsyncEtcd::ServiceDiscovery discovery("http://127.0.0.1:2379");
    auto res = co_await discovery.Discovery("/app/api/login");
    auto m = std::any_cast<std::unordered_map<std::string,std::string>>(res);
    for(auto [k,v]:m){
        std::cout << k << ": " << v <<'\n';
    }
    co_return galay::common::CoroutineStatus::kCoroutineFinished;
}


int main()
{
    galay::common::GY_NetCoroutine<> task = func();
    getchar();
    galay::MiddleWare::AsyncEtcd::DistributedLock S_Lock("http://127.0.0.1:2379");
    S_Lock.Lock("/mutex/lock",10);
    std::cout << "lock success\n";
    getchar();
    S_Lock.UnLock();
    std::cout <<"lock release\n";
    getchar();
    return 0;
}