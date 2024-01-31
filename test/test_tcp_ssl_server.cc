#include "../galay/factory/factory.h"
#include "../galay/kernel/error.h"
#include <signal.h>

using namespace galay;


Task<> func(Task_Base::wptr t_task)
{
    auto task = t_task.lock();
    auto req = std::dynamic_pointer_cast<protocol::Tcp_Request>(task->get_req());
    auto resp = std::dynamic_pointer_cast<protocol::Tcp_Response>(task->get_resp());
    std::cout<<req->get_buffer()<<'\n';
    resp->get_buffer() = "HTTP/1.1 200 OK\r\n\
Content-Type: text/html\r\n\r\n\
<!DOCTYPE html>\
<html>\
<body>\
Hello, World!\
</body>\
</html>\r\n\r\n";
    //to send
    task->control_task_behavior(Task_Status::GY_TASK_WRITE);
    //to disconnect
    task->finish();
    return {};
}

TcpSSLServer server;

void sig_handle(int sig)
{
    server->stop();
}

int main()
{
    signal(SIGINT,sig_handle);
    auto config = Config_Factory::create_tcp_ssl_server_config(8080,TLS1_2_VERSION,TLS1_3_VERSION,"../server.crt","../server.key");
    auto scheduler = Scheduler_Factory::create_epoll_scheduler(DEFAULT_EVENT_SIZE,DEFAULT_EVENT_TIME_OUT);
    server = Server_Factory::create_tcp_ssl_server(config,scheduler);
    server->start(func);
    if(server->get_error() != error::base_error::GY_SUCCESS)
    {
       std::cout<<error::get_err_str(server->get_error())<<std::endl;
    }
    return 0;
}
