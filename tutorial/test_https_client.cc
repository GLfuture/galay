
#include "../galay/factory/factory.h" 

using namespace galay;

Task<> func(Scheduler_Base::ptr scheduler)
{
    auto client = Client_Factory::create_https_client(scheduler,TLS1_1_VERSION,TLS1_3_VERSION);
    auto ret = co_await client->connect("39.156.66.14",443);
    if(std::any_cast<int>(ret) == 0) {
        std::cout<<"connect success\n";
    }else{
        std::cout<<"connect failed\n";
    }
    Protocol::Http1_1_Request::ptr request = std::make_shared<Protocol::Http1_1_Request>();
    request->GetVersion() = "1.1";
    request->GetMethod() = "GET";
    request->GetUri() = "/";
    //request->SetHeadPair({"Connection","close"});
    Protocol::Http1_1_Response::ptr response = std::make_shared<Protocol::Http1_1_Response>();
    ret = co_await client->request(request,response);
    std::string resp_str = response->EncodePdu();
    std::cout<< resp_str << '\n' << resp_str.length() <<std::endl;
    scheduler->Stop();
    co_return;
}


int main()
{
    //auto scheduler = Scheduler_Factory::create_epoll_scheduler(1,5);
    auto scheduler = Scheduler_Factory::create_select_scheduler(5);
    Task<> t = func(scheduler);
    scheduler->Start();
    std::cout<<"end\n";
    return 0;
}
