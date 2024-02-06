#include "factory.h"

//tcp
galay::Tcp_Server_Config::ptr galay::Config_Factory::create_tcp_server_config(int port)
{
    return std::make_shared<Tcp_Server_Config>(Tcp_Server_Config(port,DEFAULT_BACKLOG, DEFAULT_RECV_LENGTH,-1));
}


galay::Tcp_Server_Config::ptr galay::Config_Factory::create_tcp_server_config(Tcp_Server_Config &&config)
{
    return std::make_shared<Tcp_Server_Config>(config);
}

galay::Tcp_Server_Config::ptr galay::Config_Factory::create_tcp_server_config(uint16_t port,uint32_t backlog,uint32_t max_rbuffer_len,int conn_timeout)
{
    return std::make_shared<Tcp_Server_Config>(port,backlog,max_rbuffer_len,conn_timeout);
}

//tcp ssl
galay::Tcp_SSL_Server_Config::ptr galay::Config_Factory::create_tcp_ssl_server_config(int port,long ssl_min_version,long ssl_max_version
            ,const char* cert_filepath,const char* key_filepath)
{
    return std::make_shared<Tcp_SSL_Server_Config>(Tcp_SSL_Server_Config(port,DEFAULT_BACKLOG,DEFAULT_RECV_LENGTH,-1
        ,DEFAULT_MAX_SSL_ACCEPT_RETRY,ssl_min_version,ssl_max_version,cert_filepath,key_filepath));
}


galay::Tcp_SSL_Server_Config::ptr galay::Config_Factory::create_tcp_ssl_server_config(Tcp_SSL_Server_Config &&config)
{
    return std::make_shared<Tcp_SSL_Server_Config>(config);
}

galay::Tcp_SSL_Server_Config::ptr galay::Config_Factory::create_tcp_ssl_server_config(uint16_t port,uint32_t backlog,uint32_t max_rbuffer_len,int conn_timeout
            , long ssl_min_version , long ssl_max_version, uint32_t ssl_max_accept_retry , const char* cert_filepath, const char* key_filepath)
{
    return std::make_shared<Tcp_SSL_Server_Config>(port,backlog,max_rbuffer_len,conn_timeout,ssl_min_version,ssl_max_version,ssl_max_accept_retry ,cert_filepath,key_filepath);
}


//http
galay::Http_Server_Config::ptr galay::Config_Factory::create_http_server_config(int port)
{
    return std::make_shared<Http_Server_Config>(port,DEFAULT_BACKLOG,DEFAULT_RECV_LENGTH,-1);
}


galay::Http_Server_Config::ptr galay::Config_Factory::create_http_server_config(Http_Server_Config &&config)
{
    return std::make_shared<Http_Server_Config>(config);
}

galay::Http_Server_Config::ptr galay::Config_Factory::create_http_server_config(uint16_t port,uint32_t backlog,uint32_t max_rbuffer_len,int conn_timeout)
{
    return std::make_shared<Http_Server_Config>(port,backlog,max_rbuffer_len,conn_timeout);
}

//https

galay::Https_Server_Config::ptr galay::Config_Factory::create_https_server_config(int port,long ssl_min_version,long ssl_max_version
            , const char* cert_filepath,const char* key_filepath)
{
    return std::make_shared<Https_Server_Config>(port,DEFAULT_BACKLOG,DEFAULT_RECV_LENGTH,-1,ssl_min_version,ssl_max_version
        ,DEFAULT_MAX_SSL_ACCEPT_RETRY,cert_filepath,key_filepath);
}

galay::Https_Server_Config::ptr galay::Config_Factory::create_https_server_config(Https_Server_Config &&config)
{
    return std::make_shared<Https_Server_Config>(config);
}

galay::Https_Server_Config::ptr galay::Config_Factory::create_https_server_config(uint16_t port,uint32_t backlog,uint32_t max_rbuffer_len ,int conn_timeout
    , long ssl_min_version , long ssl_max_version , uint32_t ssl_accept_max_retry ,const char* cert_filepath,const char* key_filepath)
{
    return std::make_shared<Https_Server_Config>(port,backlog,max_rbuffer_len,conn_timeout,ssl_min_version,ssl_max_version,ssl_accept_max_retry,cert_filepath,key_filepath);
}




//server
galay::Tcp_Server<galay::protocol::Http1_1_Request,galay::protocol::Http1_1_Response>::ptr galay::Server_Factory::create_http_server(Http_Server_Config::ptr config
     ,Scheduler_Base::ptr scheduler)
{
    return std::make_shared<Tcp_Server<protocol::Http1_1_Request,protocol::Http1_1_Response>>(config,scheduler);
}

galay::Tcp_SSL_Server<galay::protocol::Http1_1_Request,galay::protocol::Http1_1_Response>::ptr galay::Server_Factory::create_https_server(Https_Server_Config::ptr config
     ,Scheduler_Base::ptr scheduler)
{
    return std::make_shared<Tcp_SSL_Server<protocol::Http1_1_Request,protocol::Http1_1_Response>>(config,scheduler);
}

//scheduler
#ifdef __linux__
galay::Epoll_Scheduler::ptr galay::Scheduler_Factory::create_epoll_scheduler(int event_num,int time_out)
{
    return std::make_shared<Epoll_Scheduler>(event_num,time_out);
}
#endif

galay::Select_Scheduler::ptr galay::Scheduler_Factory::create_select_scheduler(int time_out)
{
    return std::make_shared<Select_Scheduler>(time_out);
}

//client
galay::Tcp_Client::ptr galay::Client_Factory::create_tcp_client(Scheduler_Base::wptr scheduler)
{
    return std::make_shared<Tcp_Client>(scheduler);
}

galay::Tcp_SSL_Client::ptr galay::Client_Factory::create_tcp_ssl_client(Scheduler_Base::ptr scheduler, long ssl_min_version , long ssl_max_version)
{
    return std::make_shared<Tcp_SSL_Client>(scheduler,ssl_min_version,ssl_max_version);
}


galay::Tcp_Request_Client<galay::protocol::Http1_1_Request,galay::protocol::Http1_1_Response>::ptr galay::Client_Factory::create_http_client(Scheduler_Base::wptr scheduler)
{
    return std::make_shared<Tcp_Request_Client<galay::protocol::Http1_1_Request,galay::protocol::Http1_1_Response>>(scheduler);
}

galay::Tcp_SSL_Request_Client<galay::protocol::Http1_1_Request,galay::protocol::Http1_1_Response>::ptr galay::Client_Factory::create_https_client(Scheduler_Base::wptr scheduler, long ssl_min_version , long ssl_max_version)
{
    return std::make_shared<Tcp_SSL_Request_Client<galay::protocol::Http1_1_Request,galay::protocol::Http1_1_Response>>(scheduler,ssl_min_version,ssl_max_version);
}

galay::Udp_Client::ptr galay::Client_Factory::create_udp_client(Scheduler_Base::wptr scheduler)
{
    return std::make_shared<Udp_Client>(scheduler);
}

galay::Udp_Request_Client<galay::protocol::Dns_Request,galay::protocol::Dns_Response>::ptr galay::Client_Factory::create_dns_client(Scheduler_Base::wptr scheduler)
{
    return std::make_shared<Udp_Request_Client<galay::protocol::Dns_Request,galay::protocol::Dns_Response>>(scheduler);
}

//pool
//threadpool
galay::ThreadPool::ptr galay::Pool_Factory::create_threadpool(int num)
{
    return std::make_shared<ThreadPool>(num);
}