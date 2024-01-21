
#include "../galay/factory/factory.h" 
#include <nlohmann/json.hpp>

using namespace galay;

Task<> func(Scheduler_Base::ptr scheduler)
{
    auto client = Client_Factory::create_https_client(scheduler,TLS1_1_VERSION,TLS1_3_VERSION);
    int ret = co_await client->connect("39.156.66.14",443);
    if(ret == 0) {
        std::cout<<"connect success\n";
    }else{
        std::cout<<"connect failed\n";
    }
    Http_Request::ptr request = std::make_shared<Http_Request>();
    request->get_version() = "HTTP/1.1";
    request->get_method() = "GET";
    request->get_url_path() = "/";
    //request->set_head_kv_pair({"Connection","close"});
    Http_Response::ptr response = std::make_shared<Http_Response>();
    ret = co_await client->request(request,response);
    std::string resp_str = response->encode();
    std::cout<< resp_str << '\n' << resp_str.length() <<std::endl;
    scheduler->stop();
    co_return;
}


int main()
{
    //auto scheduler = Scheduler_Factory::create_epoll_scheduler(1,5);
    auto scheduler = Scheduler_Factory::create_select_scheduler(5);
    Task<> t = func(scheduler);
    scheduler->start();
    std::cout<<"end\n";
    return 0;
}
