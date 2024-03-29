#include "../galay/factory/factory.h"
#include "../galay/middleware/AsyncEtcd.h"

galay::Task<> func()
{
    galay::MiddleWare::AsyncEtcd::ServiceDiscovery discovery("http://127.0.0.1:2379");
    auto res = co_await discovery.Discovery("/app/api/login");
    auto m = std::any_cast<std::unordered_map<std::string,std::string>>(res);
    for(auto [k,v]:m){
        std::cout << k << ": " << v <<'\n';
    }
}


int main()
{
    galay::Task<> task = func();
    getchar();
    return 0;
}