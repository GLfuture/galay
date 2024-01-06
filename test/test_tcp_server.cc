#include "../src/factory/factory.h"
#include "../src/kernel/error.h"
#include <signal.h>
using namespace galay;

int global_time = 0;

Task<> func(Task_Base<Tcp_Request,Tcp_Response>::ptr task)
{
    std::cout<<task->get_req()->get_buffer()<<'\n';
    task->get_resp()->get_buffer() = "world!";
    //finish 完成本次任务后停止，control_task_behavior本次任务仍会执行但不会回发消息
    //if(global_time++ > 5) task->finish();
    if(global_time++ > 5) task->control_task_behavior(Task_Status::GY_TASK_DISCONNECT);
    return {};
}

void sig_handle(int sig)
{

}

int main()
{
    signal(SIGINT,sig_handle);
    auto config = Config_Factory::create_tcp_server_config(8080);
    auto scheduler = Scheduler_Factory::create_tcp_scheduler(IO_EPOLL,DEFAULT_EVENT_SIZE,DEFAULT_EVENT_TIME_OUT);
    auto server = Server_Factory::create_tcp_server(config,scheduler);
    server->start(func);
    if(server->get_error() != error::base_error::GY_SUCCESS)
    {
       std::cout<<error::get_err_str(server->get_error())<<std::endl;
    }
    return 0;
}