#include "../galay/factory/factory.h"
#include <signal.h>
using namespace galay;

Task<> func(Task_Base::wptr t_task)
{
    auto task = t_task.lock();
    auto req = std::dynamic_pointer_cast<protocol::Http1_1_Request>(task->get_req());
    auto resp = std::dynamic_pointer_cast<protocol::Http1_1_Response>(task->get_resp());
    if(task->get_scheduler() ==nullptr) std::cout<<"NULL\n";
    auto client = Client_Factory::create_http_client(task->get_scheduler());
    int ret = co_await client->connect("39.156.66.14",80);
    if(client->get_error() == error::GY_SUCCESS) std::cout<<"connect success\n";
    else std::cout<<"connect failed error is "<<client->get_error()<<'\n';
    //std::cout<<req->encode()<<'\n';
    auto t_req = std::make_shared<protocol::Http1_1_Request>();
    t_req->get_version() = "1.1";
    t_req->get_method() = "GET";
    t_req->get_url_path() = "/";
    t_req->set_head_kv_pair({"Connection","close"});
    ret = co_await client->request(t_req,resp);
    if(client->get_error() == error::GY_SUCCESS) std::cout<<"request success\n";
    else std::cout<<"request failed error is "<<client->get_error()<<'\n';
    //std::cout<<resp->encode();
    // if(!task->get_ctx().has_value()){
    //     task->get_ctx() = 1;
    // }else{
    //     int& ctx = std::any_cast<int&>(task->get_ctx());
    //     if( ++ctx == 2) {
    //         std::cout<<"task finish\n";
    //     }
    // }
    // //resp->get_status() = 200;
    //resp->get_version() = "1.1";
    //resp->set_head_kv_pair({"Connection","close"});
    //resp->get_body() = "Hello World!";
    task->control_task_behavior(Task_Status::GY_TASK_WRITE);
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
    auto config = Config_Factory::create_http_server_config(8080,10,1024,5000); //5s断连
    //auto scheduler = Scheduler_Factory::create_epoll_scheduler(1024,-1);
    auto scheduler = Scheduler_Factory::create_select_scheduler(0);
    http_server = Server_Factory::create_http_server(config,scheduler);
    http_server->start(func);
    return 0;
}