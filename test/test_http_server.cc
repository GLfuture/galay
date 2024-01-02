#include "../src/factory/factory.h"
#include <signal.h>
using namespace galay;

void func(Task_Base<Http_Request,Http_Response>::ptr task)
{
    std::cout<<task->get_req()->encode();
    task->get_resp()->get_status() = 200;
    task->get_resp()->get_version() = "HTTP/1.1";
    task->get_resp()->set_head_kv_pair({"Connection","close"});
    task->get_resp()->get_body() = "<!DOCTYPE html><html><body>Hello, World!</body></html>";
    task->control_task_behavior(Task_Status::GY_TASK_WRITE);
}

void sig_handle(int sig)
{

}

int main()
{
    signal(SIGINT,sig_handle);
    auto config = Config_Factory::create_http_server_config(8080);
    auto http_server = Server_Factory::create_http_server(config);
    http_server->start(func);
    return 0;
}