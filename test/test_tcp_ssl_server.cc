#include "../src/server/tcpserver.hpp"
#include "../src/protocol/tcp.h"
#include "../src/factory/factory.h"
#include <signal.h>
using namespace galay;


void func(Task<Tcp_Request,Tcp_Response>::ptr task)
{
    std::cout<<task->get_req()->get_buffer()<<'\n';
    task->get_resp()->get_buffer() = "HTTP/1.1 200 OK\r\n\
Content-Type: text/html\r\n\r\n\
<!DOCTYPE html>\
<html>\
<body>\
Hello, World!\
</body>\
</html>\r\n\r\n";
}

Tcp_SSL_Server<Tcp_Request,Tcp_Response>::ptr server = std::make_shared<Tcp_SSL_Server<Tcp_Request,Tcp_Response>>
    (Config_Factory::create_tcp_ssl_server_config(8080,TLS1_2_VERSION,TLS1_3_VERSION,"../server.crt","../server.key"));

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
