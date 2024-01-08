
#include "../src/factory/factory.h" 
#include <nlohmann/json.hpp>

using namespace galay;

Task<> func(IO_Scheduler::ptr scheduler)
{
    auto client = Client_Factory::create_http_client(scheduler);
    int ret = co_await client->connect("192.168.196.239",8080);
    if(ret == 0) {
        std::cout<<"connect success\n";
    }else{
        std::cout<<"connect failed\n";
    }
    Http_Request::ptr request = std::make_shared<Http_Request>();
    request->get_version() = "HTTP/1.1";
    request->get_method() = "POST";
    request->get_url_path() = "/predict";
    request->set_head_kv_pair({"Connection","close"});
    request->set_head_kv_pair({"Content-Type","application/json"});
    nlohmann::json js;
    js["model"] = "RNN";
    js["seq_len"] = 10;
    js["pred_len"] = 10;
    js["target"] = "press";
    js["enc_in"] = 8;
    js["dec_in"] = 8;
    js["e_layers"] = 4;
    js["d_layers"] = 1;
    request->get_body() = js.dump();  
    std::cout<<request->encode()<<'\n';
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
