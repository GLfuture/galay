#include "galay/middleware/Etcd.h"
#include "galay/galay.h"
#include <iostream>

galay::coroutine::Coroutine func()
{
    galay::etcd::EtcdClient client("http://127.0.0.1:2379", 0);
    auto res = co_await client.DiscoverServicePrefix("/app/api/login");
    auto resp = client.GetResponse();
    auto result = resp.GetKeyValues();
    for(auto& item : result)
    {
        std::cout << item.first << ":" << item.second << std::endl;
    }
    client.Watch("/app/api/login", [](::etcd::Response res){
        std::cout << res.value().as_string() << std::endl;
    });
    getchar();
    co_return;
}


int main()
{
    galay::InitializeGalayEnv({1, -1}, {0, -1}, {0, -1});
    func();
    getchar();
    galay::DestroyGalayEnv();
    return 0;
}