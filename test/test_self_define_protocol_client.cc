
#include "../src/factory/factory.h" 

using namespace galay;

struct self_head
{
    char version[4];
    int length;
};

Task<> func(Epoll_Scheduler::ptr scheduler)
{
    auto client = Client_Factory::create_tcp_client(scheduler);
    int ret = co_await client->connect("127.0.0.1",8080);
    if(ret == 0) {
        std::cout<<"connect success\n";
    }else{
        std::cout<<"connect failed\n";
    }
    self_head head;
    memcpy(head.version,"1.1",4);
    char t_head[sizeof(self_head)];
    char *buffer = new char[20];
    for(int i = 0 ; i <= 10000 ; i++)
    {
        std::string wbuffer = std::to_string(i) + ": hello world\n";
        head.length = wbuffer.length();
        memcpy(t_head,&head,sizeof(self_head));
        std::string sendmsg(t_head,sizeof(self_head));
        sendmsg+=wbuffer;
        ret = co_await client->send(sendmsg,sendmsg.length());
        memset(buffer,0,20);
        ret = co_await client->recv(buffer,sizeof(self_head));
        self_head t_head;
        memcpy(&t_head,buffer,sizeof(self_head));
        memset(buffer,0,20);
        ret = co_await client->recv(buffer,t_head.length);
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
