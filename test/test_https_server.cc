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
    auto config = Config_Factory::create_https_server_config(8080,TLS1_2_VERSION,TLS1_3_VERSION,"../server.crt","../server.key");
    auto scheduler = Scheduler_Factory::create_http_scheduler(IO_EPOLL,DEFAULT_EVENT_SIZE,DEFAULT_EVENT_TIME_OUT);
    auto https_server = Server_Factory::create_https_server(config,scheduler);
    //auto config2 = Config_Factory::create_https_server_config(8081,TLS1_2_VERSION,TLS1_3_VERSION,"../server.crt","../server.key");
    //auto https_server2 = Server_Factory::create_https_server(config2,scheduler);
    std::cout<<errno<<std::endl;
    https_server->start(func);
    return 0;
}