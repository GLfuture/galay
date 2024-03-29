#include "../galay/middleware/AsyncEtcd.h"

int main()
{
    galay::MiddleWare::AsyncEtcd::ServiceRegister S_Register("http://127.0.0.1:2379");
    S_Register.Register("/app/api/login/node4","127.0.0.5:8080",10);
    getchar();
    return 0;
}