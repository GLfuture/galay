#include "../src/protocol/http.h"
#include "../src/server/httpserver.hpp"
#include "../src/factory/factory.h"
#include <signal.h>
using namespace galay;

void func(Task<Http_Request,Http_Response>::ptr task)
{
    std::cout<<task->get_req()->encode();
    task->get_resp()->get_status() = 200;
    task->get_resp()->get_version() = "HTTP/1.1";
    task->get_resp()->set_head_kv_pair({"Connection","close"});
    task->get_resp()->get_body() = "<!DOCTYPE html><html><body>Hello, World!</body></html>";
    std::cout << task->get_resp()->encode()<<'\n';
}

Http_Server<Http_Request,Http_Response>::ptr http_server = std::make_shared<Http_Server<Http_Request,Http_Response>>(Config_Factory::create_http_server_config(Http_Server_Config(8080,10,IO_EPOLL)));

void signal_handle(int sign)
{
    http_server->stop();
}

int main()
{
    signal(SIGINT,signal_handle);
    http_server->start(func);
    return 0;
}