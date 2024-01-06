#include "../src/factory/factory.h"
#include <signal.h>
using namespace galay;

Task<> func(Task_Base::ptr task)
{
    auto req = std::dynamic_pointer_cast<Http_Request>(task->get_req());
    auto resp = std::dynamic_pointer_cast<Http_Response>(task->get_resp());
    std::cout<<req->encode();
    resp->get_status() = 200;
    resp->get_version() = "HTTP/1.1";
    resp->set_head_kv_pair({"Connection","close"});
    resp->get_body() = "<!DOCTYPE html><html><body>Hello, World!</body></html>";
    return {};
}

void sig_handle(int sig)
{

}

int main()
{
    signal(SIGINT,sig_handle);
    auto config = Config_Factory::create_http_server_config(8080);
    auto scheduler = Scheduler_Factory::create_http_scheduler(IO_EPOLL,1024,5);
    auto http_server = Server_Factory::create_http_server(config,scheduler);
    http_server->start(func);
    return 0;
}