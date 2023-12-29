#include "../src/server/tcpserver.hpp"
#include "../src/protocol/tcp.h"
#include <signal.h>
using namespace galay;

void func(Task<Tcp_Request,Tcp_Response>::ptr task)
{
    std::cout<<task->get_req()->get_buffer()<<'\n';
    task->get_resp()->get_buffer() = "world!";
    std::cout << task->get_error() << '\n';
}

Tcp_Server<Tcp_Request,Tcp_Response>::ptr server = std::make_shared<Tcp_Server<Tcp_Request,Tcp_Response>>(Tcp_Server_Config(8081,10,IO_ENGINE::IO_EPOLL));

void signal_handle(int sign)
{
    server->stop();
}

int main()
{
    signal(SIGINT,signal_handle);
    server->start(func);
    if(server->get_error() == error::server_error::GY_ENGINE_HAS_ERROR)
    {
        std::cout<<server->get_engine()->get_error()<<std::endl;
    }
    return 0;
}