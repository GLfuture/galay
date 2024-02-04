#ifndef GALAY_FACTORY_H
#define GALAY_FACTORY_H

#include "../config/config.h"
#include "../server/server.h"
#include "../client/client.h"
#include "../protocol/http1_1.h"
#include "../kernel/threadpool.h"

namespace galay
{
    using HttpServer = Tcp_Server<protocol::Http1_1_Request,protocol::Http1_1_Response>::ptr;
    using HttpsServer = Tcp_SSL_Server<protocol::Http1_1_Request,protocol::Http1_1_Response>::ptr;

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
        //http
        static HttpServer create_http_server(Http_Server_Config::ptr config ,Scheduler_Base::ptr scheduler);
        //https
        static HttpsServer create_https_server(Https_Server_Config::ptr config ,Scheduler_Base::ptr scheduler);
    };


    class Scheduler_Factory: public Factory_Base
    {
    public:
        using ptr = std::shared_ptr<Scheduler_Factory>;
#ifdef __linux__
        static Epoll_Scheduler::ptr create_epoll_scheduler(int event_num,int time_out);
#endif
        static Select_Scheduler::ptr create_select_scheduler(int time_out);
    };

    class Client_Factory: public Factory_Base
    {
    public:
        using ptr = std::shared_ptr<Client_Factory>;
        static Tcp_Client::ptr create_tcp_client(Scheduler_Base::wptr scheduler);

        static Tcp_SSL_Client::ptr create_tcp_ssl_client(Scheduler_Base::ptr scheduler, long ssl_min_version , long ssl_max_version);

        static Tcp_Request_Client<protocol::Http1_1_Request,protocol::Http1_1_Response>::ptr create_http_client(Scheduler_Base::wptr scheduler);

        static Tcp_SSL_Request_Client<protocol::Http1_1_Request,protocol::Http1_1_Response>::ptr create_https_client(Scheduler_Base::wptr scheduler, long ssl_min_version , long ssl_max_version);

        static Udp_Client::ptr create_udp_client(Scheduler_Base::wptr scheduler);

        static Udp_Request_Client<protocol::Dns_Request,protocol::Dns_Response>::ptr create_dns_client(Scheduler_Base::wptr scheduler);

    };
    
    //pool factory
    class Pool_Factory: public Factory_Base
    {
    public:
        static ThreadPool::ptr create_threadpool(int num);
    };
}


#endif