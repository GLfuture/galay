#include "../src/factory/factory.h"
#include <string.h>
using namespace galay;

Task<> func(IO_Scheduler::ptr scheduler)
{
    auto client = Client_Factory::create_tcp_ssl_client(scheduler,TLS1_1_VERSION,TLS1_3_VERSION);
    int ret = co_await client->connect("39.156.66.14",443);
    if(ret == 0) {
        std::cout<<"connect success\n";
    }else{
        std::cout<<"connect failed\n";
    }
    std::string req = "GET / HTTP/1.1\r\nConnection: close\r\n\r\n";
    ret = co_await client->send(req,req.length());

    char buffer[1024];
    std::string result ;
    do{
        memset(buffer,0,1024);
        ret = co_await client->recv(buffer,1024) ;
        if(ret != -1) result.append(buffer,ret);
    } while(ret != -1);
    std::cout<<result<<'\n';
    client->disconnect();
    scheduler->stop();
    co_return;
}


int main()
{
    auto scheduler = Scheduler_Factory::create_tcp_scheduler(IO_EPOLL,1,5);
    Task<> t = func(scheduler);
    scheduler->start();
    std::cout<<"end\n";
    return 0;
}
