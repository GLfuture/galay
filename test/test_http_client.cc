
#include "../galay/factory/factory.h" 
#include <nlohmann/json.hpp>

using namespace galay;

Task<> func(Scheduler_Base::ptr scheduler)
{
    auto client = Client_Factory::create_http_client(scheduler);
    int ret = co_await client->connect("39.156.66.14",80);
    if(ret == 0) {
        std::cout<<"connect success\n";
    }else{
        std::cout<<"connect failed\n";
    }
    protocol::Http1_1_Request::ptr request = std::make_shared<protocol::Http1_1_Request>();
    request->get_version() = "1.1";
    request->get_method() = "GET";
    request->get_url_path() = "/";
    request->set_head_kv_pair({"Connection","close"});
    protocol::Http1_1_Response::ptr response = std::make_shared<protocol::Http1_1_Response>();
    ret = co_await client->request(request,response);
    std::cout<<response->encode()<<std::endl;
    std::cout<<ret<<'\n';
    ret = co_await client->request(request,response);
    std::cout<<ret<<'\n';
    std::cout<<response->encode()<<std::endl;
    scheduler->stop();
    co_return;
}


int main()
{
    auto scheduler = Scheduler_Factory::create_select_scheduler(0);
    Task<> t = func(scheduler);
    scheduler->start();
    return 0;
}
