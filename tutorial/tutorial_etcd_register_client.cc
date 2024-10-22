#include "../galay/galay.h"
#include "../galay/middleware/Etcd.h"

galay::coroutine::Coroutine func()
{
    galay::middleware::etcd::EtcdClient client("http://127.0.0.1:2379", 0);
    bool res = co_await client.RegisterService("/app/api/login/node","127.0.0.1:7070");
    auto resp = client.GetResponse();
    std::cout << std::boolalpha << resp.Success() << std::endl;
    co_return;
}

int main()
{
    galay::scheduler::DynamicResizeCoroutineSchedulers(1);
    galay::scheduler::StartCoroutineSchedulers();
    func();
    getchar();
    galay::scheduler::StopCoroutineSchedulers();
    return 0;
}