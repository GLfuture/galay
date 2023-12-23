#include "../src/server/server.hpp"
#include <signal.h>
using namespace galay;

void func(Tcp_Request::ptr request,Tcp_Response::ptr response)
{
    std::cout<<request->get_buffer()<<std::endl;
    response->set_buffer(request->get_buffer());
}

Tcp_Server<Tcp_Request,Tcp_Response>::ptr server = std::make_shared<Tcp_Server<Tcp_Request,Tcp_Response>>(Tcp_Server_Config(8080,10,IO_ENGINE::IO_EPOLL));

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