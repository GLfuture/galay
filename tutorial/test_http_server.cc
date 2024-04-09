#include "../galay/factory/factory.h"
#include "../galay/kernel/callback.h"
#include <signal.h>
using namespace galay;


Task<> func(Task_Base::wptr t_task)
{
    auto task = t_task.lock();
    auto req = std::dynamic_pointer_cast<Protocol::Http1_1_Request>(task->get_req());
    auto resp = std::dynamic_pointer_cast<Protocol::Http1_1_Response>(task->get_resp());
    std::cout << req->get_body() <<'\n';
    if(task->get_scheduler() == nullptr) std::cout<<"NULL\n";
    auto client = Client_Factory::create_http_client(task->get_scheduler());
    auto ret = co_await client->connect("39.156.66.14",80);
    if(client->get_error() == Error::GY_SUCCESS) std::cout<<"connect success\n";
    else std::cout<<"connect failed error is "<<client->get_error()<<'\n';
    //std::cout<<req->encode()<<'\n';
    auto t_req = std::make_shared<Protocol::Http1_1_Request>();
    t_req->get_version() = "1.1";
    t_req->get_method() = "GET";
    t_req->get_url_path() = "/";
    t_req->set_head_kv_pair({"Connection","close"});
    ret = co_await client->request(t_req,resp);
    if(client->get_error() == Error::GY_SUCCESS) std::cout<<"request success\n";
    else std::cout<<"request failed error is "<<client->get_error()<<'\n';
    task->control_task_behavior(Task_Status::GY_TASK_WRITE);
    // if(req->get_head_value("Connection").compare("close") == 0) {
    //     std::cout << "close\n";
    // }
    task->finish(); 
    co_return;
}

HttpServer http_server;

void sig_handle(int sig)
{
    http_server->stop();
}

int main()
{
    signal(SIGINT,sig_handle);
    Callback_ConnClose::set([](int fd){
        std::cout << "exit :" << fd << "\n";  
    });
    auto config = Config_Factory::create_http_server_config(Engine_Type::ENGINE_EPOLL,5,3); //5s断
    config->set_conn_timeout(5000);     //5s断
    http_server = Server_Factory::create_http_server(config);
    http_server->start({{8010,func}});
    return 0;
}