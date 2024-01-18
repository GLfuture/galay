#ifndef GALAY_FACTORY_H
#define GALAY_FACTORY_H

#include "../config/config.h"
#include "../server/server.h"
#include "../client/client.h"
#include "../protocol/tcp.h"
#include "../protocol/http.h"
#include "../kernel/threadpool.h"

namespace galay
{

    using TcpServer = Tcp_Server<Tcp_Request,Tcp_Response>::ptr;
    using TcpSSLServer = Tcp_SSL_Server<Tcp_Request,Tcp_Response>::ptr;
    using HttpServer = Tcp_Server<Http_Request,Http_Response>::ptr;
    using HttpsServer = Tcp_SSL_Server<Http_Request,Http_Response>::ptr;

    class Factory_Base
    {
    public:
        
    };

    class Config_Factory:public Factory_Base
    {
    public:
        using ptr = std::shared_ptr<Config_Factory>;
        //tcp 
        static Tcp_Server_Config::ptr create_tcp_server_config(int port);
        static Tcp_Server_Config::ptr create_tcp_server_config(Tcp_Server_Config &&config);
        static Tcp_Server_Config::ptr create_tcp_server_config(uint16_t port,uint32_t backlog,uint32_t max_rbuffer_len);

        //tcp ssl
        static Tcp_SSL_Server_Config::ptr create_tcp_ssl_server_config(int port,long ssl_min_version,long ssl_max_version
            , const char* cert_filepath,const char* key_filepath);
        static Tcp_SSL_Server_Config::ptr create_tcp_ssl_server_config(Tcp_SSL_Server_Config &&config);
        static Tcp_SSL_Server_Config::ptr create_tcp_ssl_server_config(uint16_t port,uint32_t backlog,uint32_t max_rbuffer_len
            , long ssl_min_version , long ssl_max_version ,uint32_t ssl_max_accept_retry,const char* cert_filepath, const char* key_filepath);

        //http
        static Http_Server_Config::ptr create_http_server_config(int port);
        static Http_Server_Config::ptr create_http_server_config(Http_Server_Config &&config);
        static Http_Server_Config::ptr create_http_server_config(uint16_t port,uint32_t backlog,uint32_t max_rbuffer_len);


        //https
        static Https_Server_Config::ptr create_https_server_config(int port,long ssl_min_version,long ssl_max_version
            , const char* cert_filepath,const char* key_filepath);
        static Https_Server_Config::ptr create_https_server_config(Https_Server_Config &&config);
        static Https_Server_Config::ptr create_https_server_config(uint16_t port,uint32_t backlog,uint32_t max_rbuffer_len
            , long ssl_min_version , long ssl_max_version, uint32_t ssl_accept_max_retry ,const char* cert_filepath,const char* key_filepath);
        
    };

    class Server_Factory:public Factory_Base
    {
    public:
        using ptr = std::shared_ptr<Server_Factory>;
        //tcp
        static TcpServer create_tcp_server(Tcp_Server_Config::ptr config , Epoll_Scheduler::ptr scheduler);
        //tcp ssl
        static TcpSSLServer create_tcp_ssl_server(Tcp_SSL_Server_Config::ptr config ,Epoll_Scheduler::ptr scheduler);
        //http
        static HttpServer create_http_server(Http_Server_Config::ptr config ,Epoll_Scheduler::ptr scheduler);
        //https
        static HttpsServer create_https_server(Https_Server_Config::ptr config ,Epoll_Scheduler::ptr scheduler);
    };


    class Scheduler_Factory: public Factory_Base
    {
    public:
        using ptr = std::shared_ptr<Scheduler_Factory>;
        static Epoll_Scheduler::ptr create_epoll_scheduler(int event_num,int time_out);
    };

    class Client_Factory: public Factory_Base
    {
    public:
        using ptr = std::shared_ptr<Client_Factory>;
        static Tcp_Client::ptr create_tcp_client(Epoll_Scheduler::wptr scheduler);

        static Tcp_SSL_Client::ptr create_tcp_ssl_client(Epoll_Scheduler::ptr scheduler, long ssl_min_version , long ssl_max_version);

        static Http_Client::ptr create_http_client(Epoll_Scheduler::wptr scheduler);

        static Https_Client::ptr create_https_client(Epoll_Scheduler::ptr scheduler, long ssl_min_version , long ssl_max_version);
    };

    class Timer_Factory: public Factory_Base
    {
    public:
        static Timer::ptr create_timer(uint64_t during , uint32_t exec_times, std::function<void()> &&func);
    };

    //pool factory
    class Pool_Factory: public Factory_Base
    {
    public:
        static ThreadPool::ptr create_threadpool(int num);
    };
}


#endif