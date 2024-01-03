#include "../src/factory/factory.h"
#include "../src/kernel/error.h"
#include <signal.h>
using namespace galay;

int global_time = 0;

Task<> func(Task_Base<Tcp_Request,Tcp_Response>::ptr task)
{
    std::cout<<task->get_req()->get_buffer()<<'\n';
    task->get_resp()->get_buffer() = "world!";
    //finish 完成本次任务后停止，control_task_behavior本次任务不会执行直接停止
    if(global_time++ > 5) task->finish();
    return {};
}

void sig_handle(int sig)
{

}

int main()
{
    signal(SIGINT,sig_handle);
    auto config = Config_Factory::create_tcp_server_config(8080);
    auto server = Server_Factory::create_tcp_server(config);
    server->start(func);
    if(server->get_error() == error::server_error::GY_ENGINE_HAS_ERROR)
    {
       std::cout<<error::get_err_str(server->get_scheduler()->m_engine->get_error())<<std::endl;
    }
    return 0;
}