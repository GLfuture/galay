#include "../src/factory/factory.h"
#include "../src/kernel/error.h"
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

void sig_handle(int sig)
{

}

int main()
{
    signal(SIGINT,sig_handle);
    auto config = Config_Factory::create_tcp_ssl_server_config(8080,TLS1_2_VERSION,TLS1_3_VERSION,"../server.crt","../server.key");
    auto server = Server_Factory::create_tcp_ssl_server(config);
    server->start(func);
    if(server->get_error() == error::server_error::GY_ENGINE_HAS_ERROR)
    {
        std::cout<<error::get_err_str(server->get_engine()->get_error())<<std::endl;
    }
    return 0;
}
