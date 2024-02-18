#ifndef GALAY_FACTORY_H
#define GALAY_FACTORY_H

#include "../config/config.h"
#include "../server/server.h"
#include "../client/client.h"
#include "../protocol/http1_1.h"
#include "../protocol/smtp.h"
#include "../kernel/threadpool.h"

namespace galay
{
    using HttpServer = Tcp_Server<protocol::Http1_1_Request,protocol::Http1_1_Response>::uptr;
    using HttpsServer = Tcp_SSL_Server<protocol::Http1_1_Request,protocol::Http1_1_Response>::uptr;

    class Factory_Base
    {
    public:
        
    };

    class Config_Factory:public Factory_Base
    {
    public:
        using ptr = std::shared_ptr<Config_Factory>;
        //tcp 
        /*
        @param port 端口
        @param type scheduler 类型
        @param sche_wait_time scheduler 返回间隔时间
        @param conn_timeout tcp连接超时时间(自动断开)
        */
        static Tcp_Server_Config::ptr create_tcp_server_config(int port,Engine_Type type, int sche_wait_time = -1,int conn_timeout = -1 , int threadnum = 1);
        static Tcp_Server_Config::ptr create_tcp_server_config(Tcp_Server_Config &&config);
        static Tcp_Server_Config::ptr create_tcp_server_config(uint16_t port,uint32_t backlog,uint32_t max_rbuffer_len,Engine_Type type, int sche_wait_time,int max_event_size,int conn_timeout, int threadnum);

        //tcp ssl
        static Tcp_SSL_Server_Config::ptr create_tcp_ssl_server_config(int port,long ssl_min_version,long ssl_max_version
            , const char* cert_filepath,const char* key_filepath,Engine_Type type, int sche_wait_time = -1,int conn_timeout = -1, int threadnum = 1);
        static Tcp_SSL_Server_Config::ptr create_tcp_ssl_server_config(Tcp_SSL_Server_Config &&config);
        static Tcp_SSL_Server_Config::ptr create_tcp_ssl_server_config(uint16_t port,uint32_t backlog,uint32_t max_rbuffer_len, long ssl_min_version , long ssl_max_version
             ,uint32_t ssl_max_accept_retry,const char* cert_filepath, const char* key_filepath,Engine_Type type,int sche_wait_time,int max_event_size,int conn_timeout, int threadnum);

        //http
        static Http_Server_Config::ptr create_http_server_config(int port,Engine_Type type, int sche_wait_time = -1,int conn_timeout = -1, int threadnum = 1);
        static Http_Server_Config::ptr create_http_server_config(Http_Server_Config &&config);
        static Http_Server_Config::ptr create_http_server_config(uint16_t port,uint32_t backlog,uint32_t max_rbuffer_len,Engine_Type type,int sche_wait_time,int max_event_size,int conn_timeout, int threadnum );


        //https
        static Https_Server_Config::ptr create_https_server_config(int port,long ssl_min_version,long ssl_max_version
            , const char* cert_filepath,const char* key_filepath,Engine_Type type, int sche_wait_time = -1,int conn_timeout = -1, int threadnum = 1);
        static Https_Server_Config::ptr create_https_server_config(Https_Server_Config &&config);
        static Https_Server_Config::ptr create_https_server_config(uint16_t port,uint32_t backlog,uint32_t max_rbuffer_len, long ssl_min_version , long ssl_max_version, 
            uint32_t ssl_accept_max_retry ,const char* cert_filepath,const char* key_filepath,Engine_Type type,int sche_wait_time,int max_event_size,int conn_timeout, int threadnum);
        
    };

    class Server_Factory:public Factory_Base
    {
    public:
        using ptr = std::shared_ptr<Server_Factory>;
        //http
        static HttpServer create_http_server(Http_Server_Config::ptr config );
        //https
        static HttpsServer create_https_server(Https_Server_Config::ptr config );
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
        //tcp
        //rw client
        static Tcp_Client::uptr create_tcp_client(Scheduler_Base::wptr scheduler);
        static Tcp_SSL_Client::uptr create_tcp_ssl_client(Scheduler_Base::ptr scheduler, long ssl_min_version , long ssl_max_version);

        //request client
        static Tcp_Request_Client::uptr create_http_client(Scheduler_Base::wptr scheduler);
        static Tcp_SSL_Request_Client::uptr create_https_client(Scheduler_Base::wptr scheduler, long ssl_min_version , long ssl_max_version);

        static Tcp_Request_Client::uptr create_smtp_client(Scheduler_Base::wptr scheduler);
        static Tcp_SSL_Request_Client::uptr create_smtps_client(Scheduler_Base::wptr scheduler, long ssl_min_version , long ssl_max_version);

        static Tcp_Request_Client::uptr create_tcp_self_define_client(Scheduler_Base::wptr scheduler);
        static Tcp_SSL_Request_Client::uptr create_tcp_ssl_self_define_client(Scheduler_Base::wptr scheduler, long ssl_min_version , long ssl_max_version);

        //udp 
        //rw client
        static Udp_Client::uptr create_udp_client(Scheduler_Base::wptr scheduler);

        //request client
        static Udp_Request_Client::uptr create_dns_client(Scheduler_Base::wptr scheduler);

    };
    
    //pool factory
    class Pool_Factory: public Factory_Base
    {
    public:
        static ThreadPool::ptr create_threadpool(int num);
    };
}


#endif