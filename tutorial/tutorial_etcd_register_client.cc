#include "galay/galay.hpp"
#include "galay/middleware/Etcd.hpp"

galay::Coroutine<void> func(galay::RoutineCtx ctx)
{
    galay::etcd::EtcdClient client("http://127.0.0.1:2379", 0);
    bool res = co_await client.RegisterService<void>("/app/api/login/node","127.0.0.1:7070");
    auto resp = client.GetResponse();
    std::cout << std::boolalpha << resp.Success() << std::endl;
    co_return;
}

int main()
{
    galay::InitializeGalayEnv({1, -1}, {0, -1}, {0, -1});
    func({});
    getchar();
    galay::DestroyGalayEnv();
    return 0;
}