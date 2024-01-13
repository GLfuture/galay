#include "../src/factory/factory.h"
#include <signal.h>
using namespace galay;

Task<> func(Task_Base::wptr t_task)
{
    auto task = t_task.lock();
    auto req = std::dynamic_pointer_cast<Http_Request>(task->get_req());
    // std::cout<<req->encode();
    auto resp = std::dynamic_pointer_cast<Http_Response>(task->get_resp());
    if(task->get_scheduler() ==nullptr) std::cout<<"NULL\n";
    auto client = Client_Factory::create_http_client(task->get_scheduler());
    int ret = co_await client->connect("39.156.66.14",80);
    if(client->get_error() == error::GY_SUCCESS) std::cout<<"connect success\n";
    else std::cout<<"connect failed error is "<<client->get_error()<<'\n';
    //std::cout<<req->encode()<<'\n';
    auto t_req = std::make_shared<Http_Request>();
    t_req->get_version() = "HTTP/1.1";
    t_req->get_method() = "GET";
    t_req->get_url_path() = "/";
    t_req->set_head_kv_pair({"Connection","close"});
    ret = co_await client->request(t_req,resp);
    if(client->get_error() == error::GY_SUCCESS) std::cout<<"request success\n";
    else std::cout<<"request failed error is "<<client->get_error()<<'\n';
    task->finish();
    co_return;
}

void sig_handle(int sig)
{

}

int main()
{
    signal(SIGINT,sig_handle);
    auto config = Config_Factory::create_http_server_config(8080);
    auto scheduler = Scheduler_Factory::create_scheduler(IO_EPOLL,1024,5);
    auto http_server = Server_Factory::create_http_server(config,scheduler);
    http_server->start(func);
    return 0;
}