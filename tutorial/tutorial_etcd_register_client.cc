#include "../galay/middleware/etcd.h"

int main()
{
    galay::middleware::etcd::ServiceRegister S_Register("http://127.0.0.1:2379");
    S_Register.Register("/app/api/login/node4","127.0.0.5:8080", 10);
    getchar();
    galay::middleware::etcd::DistributedLock S_Lock("http://127.0.0.1:2379");
    S_Lock.Lock("/mutex/lock",10);
    std::cout << "lock success\n";
    getchar();
    S_Lock.UnLock();
    std::cout << "lock release\n";
    getchar();
    return 0;
}