
#include "../src/factory/factory.h" 

using namespace galay;

auto scheduler = Scheduler_Factory::create_tcp_scheduler(IO_EPOLL,1,5);

Task<> func(IO_Scheduler<galay::Tcp_Request,galay::Tcp_Response>::ptr scheduler)
{
    auto client = Client_Factory::create_tcp_client(scheduler);
    int ret = co_await client->connect("127.0.0.1",8080);
    if(ret == 0) {
        std::cout<<"connect success\n";
    }else{
        std::cout<<"connect failed\n";
    }
    std::string wbuffer = "hello world\n";
    ret = co_await client->send(wbuffer,wbuffer.length());
    std::cout<<"send len :"<<ret<<'\n';
    char* buffer = new char[20];
    ret = co_await client->recv(buffer,20);
    std::cout<<"recv len :"<<ret <<"buffer: "<<buffer<<'\n';
    client->disconnect();
    scheduler->stop();
    co_return;
}


int main()
{
    epoll_event *events;
    
    Task<> t = func(scheduler);
    scheduler->start();
    std::cout<<"end\n";
    return 0;
}
