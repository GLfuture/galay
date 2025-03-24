#include "galay/middleware/Etcd.hpp"
#include "galay/galay.hpp"
//middleware需在galay.hpp之前引入
#include <iostream>

galay::Coroutine<void> func(galay::RoutineCtx ctx)
{
    galay::etcd::EtcdClient client("http://127.0.0.1:2379");
    auto res = co_await client.DiscoverServicePrefix<void>("/app/api/login");
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
    galay::GalayEnvConf conf;
    conf.m_coroutineSchedulerConf.m_thread_num = 1;
    conf.m_eventSchedulerConf.m_thread_num = 1;
    galay::GalayEnv env(conf);
    func(galay::RoutineCtx::Create(galay::EventSchedulerHolder::GetInstance()->GetScheduler(0)));
    getchar();
    galay::DestroyGalayEnv();
    return 0;
}