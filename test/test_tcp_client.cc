
#include "../src/factory/factory.h" 

using namespace galay;

Task<> func(Epoll_Scheduler::ptr scheduler)
{
    auto client = Client_Factory::create_tcp_client(scheduler);
    int ret = co_await client->connect("127.0.0.1",8080);
    if(ret == 0) {
        std::cout<<"connect success\n";
    }else{
        std::cout<<"connect failed\n";
    }
    char *buffer = new char[20];
    for(int i = 0 ; i <= 10000 ; i++)
    {
        std::string wbuffer = std::to_string(i) + ": hello world\n";
        ret = co_await client->send(wbuffer, wbuffer.length());
        memset(buffer,0,20);
        ret = co_await client->recv(buffer, 6);
        if(i % 1000 == 0) std::cout << i << "  recv len :" << ret << "buffer: " << buffer << '\n';
    }
    delete[] buffer;
    scheduler->stop();
    co_return;
}


int main()
{
    auto scheduler = Scheduler_Factory::create_epoll_scheduler(1,5);
    //auto threadpool = Pool_Factory::create_threadpool(4);
    Task<> t = func(scheduler);
    scheduler->start();
    std::cout<<"end\n";
    return 0;
}
