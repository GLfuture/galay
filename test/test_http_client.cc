
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
    request->get_version() = "HTTP/1.1";
    request->get_method() = "GET";
    request->get_url_path() = "/";
    //request->set_head_kv_pair({"Connection","close"});
    protocol::Http1_1_Response::ptr response = std::make_shared<protocol::Http1_1_Response>();
    ret = co_await client->request(request,response);
    std::cout<<ret<<'\n';
    std::cout<<response->encode()<<std::endl;
    scheduler->stop();
    co_return;
}


int main()
{
    //auto scheduler = Scheduler_Factory::create_epoll_scheduler(1,5);
    protocol::Http1_1_Request req;
    req.get_method() = "POST";
    req.get_url_path() = "/api/login";
    req.get_version() = "1.1";
    std::string body = "hello world!";
    req.get_body() = body;
    req.set_head_kv_pair({"Content-Length",std::to_string(body.length())});
    std::cout<<req.encode();
    //auto scheduler = Scheduler_Factory::create_select_scheduler(5);
    //Task<> t = func(scheduler);
    //scheduler->start();
    return 0;
}
