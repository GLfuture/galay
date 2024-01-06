
#include "../src/factory/factory.h" 

using namespace galay;

Task<> func(IO_Scheduler::ptr scheduler)
{
    auto client = Client_Factory::create_http_client(scheduler);
    int ret = co_await client->connect("39.156.66.14",80);
    if(ret == 0) {
        std::cout<<"connect success\n";
    }else{
        std::cout<<"connect failed\n";
    }
    Http_Request::ptr request = std::make_shared<Http_Request>();
    request->get_version() = "HTTP/1.1";
    request->get_method() = "GET";
    request->get_url_path() = "/";
    request->set_head_kv_pair({"Connection","close"});
    Http_Response::ptr response = std::make_shared<Http_Response>();
    co_await client->request(request,response);
    std::cout<<response->encode()<<std::endl;
    client->disconnect();
    scheduler->stop();
    co_return;
}


int main()
{
    auto scheduler = Scheduler_Factory::create_http_scheduler(IO_EPOLL,1,5);
    Task<> t = func(scheduler);
    scheduler->start();
    std::cout<<"end\n";
    return 0;
}
