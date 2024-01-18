#include "../src/factory/factory.h"
#include "../src/kernel/error.h"
#include <signal.h>
using namespace galay;

Task<> func(Task_Base::wptr t_task)
{
    auto task = t_task.lock();
    if(!task->get_ctx()){
        int *a = new int(0);
        task->set_ctx(a);
    }
    auto ctx = (int *)(task->get_ctx());
    if ((*ctx)++ >= 5)
    {
        task->control_task_behavior(Task_Status::GY_TASK_DISCONNECT);
        delete ctx;
        task->set_ctx(nullptr);
        task->finish();
        return {};
    }
    auto req = std::dynamic_pointer_cast<Tcp_Request>(task->get_req());
    auto resp = std::dynamic_pointer_cast<Tcp_Response>(task->get_resp());
    std::cout<<req->get_buffer()<<'\n';
    resp->get_buffer() = "world!";
    task->finish();
    return {};
}

void sig_handle(int sig)
{

}

int main()
{
    signal(SIGINT,sig_handle);
    auto config = Config_Factory::create_tcp_server_config(8080);
    auto scheduler = Scheduler_Factory::create_epoll_scheduler(DEFAULT_EVENT_SIZE,DEFAULT_EVENT_TIME_OUT);
    auto server = Server_Factory::create_tcp_server(config,scheduler);
    config->enable_keepalive(5,5,3);
    server->start(func);
    if(server->get_error() != error::base_error::GY_SUCCESS)
    {
       std::cout<<error::get_err_str(server->get_error())<<std::endl;
    }
    return 0;
}