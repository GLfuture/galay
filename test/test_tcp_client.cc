
#include "../src/factory/factory.h" 
using namespace galay;

Task<> func(IO_Scheduler<galay::Tcp_Request,galay::Tcp_Response>::ptr scheduler)
{
    auto client = Client_Factory::create_tcp_client(scheduler);
    int ret = co_await client->connect("39.156.66.18",80);
    std::cout<<ret<<'\n';
    client->disconnect();
    co_return;
}


int main()
{
    epoll_event *events;
    auto scheduler = Scheduler_Factory::create_tcp_scheduler(IO_EPOLL,1,5);
    Task<> t = func(scheduler);
    while(1)
    {
        scheduler->m_engine->event_check();
        int nready = scheduler->m_engine->get_active_event_num();
        events = (epoll_event*)scheduler->m_engine->result();
        for(int i = 0 ; i < nready ; i++)
        {
            scheduler->m_tasks->at(events[i].data.fd)->exec();
        }
        if(scheduler->m_tasks->empty()) break;
    }
    std::cout<<"end\n";
    return 0;
}
