#include "galay/middleware/Etcd.hpp"
#include "galay/galay.hpp"

galay::Coroutine<void> func(galay::RoutineCtx ctx)
{
    galay::etcd::EtcdClient client("http://127.0.0.1:2379");
    bool res = co_await client.RegisterService<void>("/app/api/login/node","127.0.0.1:7070");
    auto resp = client.GetResponse();
    std::cout << std::boolalpha << resp.Success() << std::endl;
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
    return 0;
}