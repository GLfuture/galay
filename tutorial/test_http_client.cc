
#include "../galay/factory/factory.h" 

using namespace galay;

Task<> func(Scheduler_Base::ptr scheduler)
{
    auto client = Client_Factory::create_http_client(scheduler);
    auto ret = co_await client->connect("39.156.66.14",80);
    if(std::any_cast<int>(ret) == 0) {
        std::cout<<"connect success\n";
    }else{
        std::cout<<"connect failed\n";
    }
    Protocol::Http1_1_Request::ptr request = std::make_shared<Protocol::Http1_1_Request>();
    request->GetVersion() = "1.1";
    request->GetMethod() = "GET";
    request->GetUri() = "/";
    Protocol::Http1_1_Response::ptr response = std::make_shared<Protocol::Http1_1_Response>();
    ret = co_await client->request<Protocol::Http1_1_Request,Protocol::Http1_1_Response>(request,response);
    std::cout<<response->EncodePdu()<<std::endl;
    std::cout<<std::any_cast<int>(ret)<<'\n';
    request->SetHeadPair({"Connection","close"});
    ret = co_await client->request<Protocol::Http1_1_Request,Protocol::Http1_1_Response>(request,response);
    std::cout<<std::any_cast<int>(ret)<<'\n';
    std::cout<<response->EncodePdu()<<std::endl;
    scheduler->Stop();
    co_return;
}

void sig_handle(int sig)
{

}


int main()
{
    auto scheduler = Scheduler_Factory::create_epoll_scheduler(1024,-1);
    Task<> t = func(scheduler);
    scheduler->Start();
    return 0;
}
