#include "../src/factory/factory.h"
#include "../src/kernel/error.h"
#include <signal.h>
using namespace galay;

void func(Task_Base<Tcp_Request,Tcp_Response>::ptr task)
{
    std::cout<<task->get_req()->get_buffer()<<'\n';
    task->get_resp()->get_buffer() = "world!";
    task->control_task_behavior(Task_Status::GY_TASK_STOP);
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