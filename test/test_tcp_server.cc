#include "../galay/factory/factory.h"
#include "../galay/kernel/error.h"
#include <signal.h>
using namespace galay;

Task<> func(Task_Base::wptr t_task)
{
    auto task = t_task.lock();
    if(!task->get_ctx().has_value()){
        task->get_ctx() = 0;
    }
    int& ctx = std::any_cast<int&>(task->get_ctx());
    auto req = std::dynamic_pointer_cast<protocol::Tcp_Request>(task->get_req());
    auto resp = std::dynamic_pointer_cast<protocol::Tcp_Response>(task->get_resp());
    if( ctx++ % 1000 == 0) std::cout<<"i :" << ctx << "  " <<req->get_buffer();
    resp->get_buffer() = "world!";
    task->control_task_behavior(Task_Status::GY_TASK_WRITE);
    return {};
}

TcpServer server;

void sig_handle(int sig)
{
    server->stop();
}

int main()
{
    signal(SIGINT,sig_handle);
    auto config = Config_Factory::create_tcp_server_config(8080);
    auto scheduler = Scheduler_Factory::create_epoll_scheduler(DEFAULT_EVENT_SIZE,DEFAULT_EVENT_TIME_OUT);
    server = Server_Factory::create_tcp_server(config,scheduler);
    config->enable_keepalive(5,5,3);
    server->start(func);
    if(server->get_error() != error::base_error::GY_SUCCESS)
    {
       std::cout<<error::get_err_str(server->get_error())<<std::endl;
    }
    return 0;
}