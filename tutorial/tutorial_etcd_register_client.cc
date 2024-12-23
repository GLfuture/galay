#include "galay/galay.h"
#include "galay/middleware/Etcd.h"

galay::Coroutine func()
{
    galay::etcd::EtcdClient client("http://127.0.0.1:2379", 0);
    bool res = co_await client.RegisterService("/app/api/login/node","127.0.0.1:7070");
    auto resp = client.GetResponse();
    std::cout << std::boolalpha << resp.Success() << std::endl;
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