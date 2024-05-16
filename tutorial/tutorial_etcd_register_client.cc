#include "../galay/middleware/AsyncEtcd.h"

int main()
{
    galay::MiddleWare::AsyncEtcd::ServiceRegister S_Register("http://127.0.0.1:2379");
    S_Register.Register("/app/api/login/node4","127.0.0.5:8080",10);
    getchar();
    galay::MiddleWare::AsyncEtcd::DistributedLock S_Lock("http://127.0.0.1:2379");
    S_Lock.Lock("/mutex/lock",10);
    ::std::cout << "lock success\n";
    getchar();
    S_Lock.UnLock();
    ::std::cout << "lock release\n";
    getchar();
    return 0;
}