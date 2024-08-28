#include "../galay/galay.h"
#include "../galay/middleware/etcd.h"

galay::coroutine::NetCoroutine func()
{
    galay::middleware::etcd::EtcdClient client("http://127.0.0.1:2379");
    auto res = client.RegisterService("/app/api/login/node4","127.0.0.5:8080");
    co_await res.Wait();
    if(!res.Success())
    {
        std::cout << res.Error() << std::endl;
    }
    galay::coroutine::NetCoroutinePool::GetInstance()->Stop();
    co_return galay::coroutine::kCoroutineFinished;
}

int main()
{
    func();
    getchar();
    if(!galay::coroutine::NetCoroutinePool::GetInstance()->IsDone()) 
    {
        galay::coroutine::NetCoroutinePool::GetInstance()->WaitForAllDone();
    }
    return 0;
}