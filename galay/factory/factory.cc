#include "factory.h"

//tcp
galay::TcpServerConf::ptr galay::Config_Factory::create_tcp_server_config(Engine_Type type,int sche_wait_time, int threadnum )
{
    return std::make_shared<TcpServerConf>(DEFAULT_BACKLOG, DEFAULT_RECV_LENGTH,type,sche_wait_time,DEFAULT_EVENT_SIZE,threadnum);
}


galay::TcpServerConf::ptr galay::Config_Factory::create_tcp_server_config(TcpServerConf &&config)
{
    return std::make_shared<TcpServerConf>(config);
}

galay::TcpServerConf::ptr galay::Config_Factory::create_tcp_server_config(uint32_t backlog,uint32_t max_rbuffer_len
    ,Engine_Type type,int sche_wait_time,int max_event_size, int threadnum)
{
    return std::make_shared<TcpServerConf>(backlog,max_rbuffer_len,type,sche_wait_time,max_event_size,threadnum);
}

//tcp ssl
galay::TcpSSLServerConf::ptr galay::Config_Factory::create_tcp_ssl_server_config(Engine_Type type,int sche_wait_time,int threadnum)
{
    return std::make_shared<TcpSSLServerConf>(DEFAULT_BACKLOG,DEFAULT_RECV_LENGTH,type,sche_wait_time,DEFAULT_EVENT_SIZE,threadnum);
}


galay::TcpSSLServerConf::ptr galay::Config_Factory::create_tcp_ssl_server_config(TcpSSLServerConf &&config)
{
    return std::make_shared<TcpSSLServerConf>(config);
}

galay::TcpSSLServerConf::ptr galay::Config_Factory::create_tcp_ssl_server_config(uint32_t backlog,uint32_t max_rbuffer_len,Engine_Type type,int sche_wait_time,int max_event_size, int threadnum)
{
    return std::make_shared<TcpSSLServerConf>(backlog,max_rbuffer_len,type,sche_wait_time,max_event_size,threadnum);
}


//http
galay::HttpServerConf::ptr galay::Config_Factory::create_http_server_config(Engine_Type type,int sche_wait_time, int threadnum)
{
    return std::make_shared<HttpServerConf>(DEFAULT_BACKLOG,DEFAULT_RECV_LENGTH,type,sche_wait_time,DEFAULT_EVENT_SIZE,threadnum);
}


galay::HttpServerConf::ptr galay::Config_Factory::create_http_server_config(HttpServerConf &&config)
{
    return std::make_shared<HttpServerConf>(config);
}

galay::HttpServerConf::ptr galay::Config_Factory::create_http_server_config(uint32_t backlog,uint32_t max_rbuffer_len
    ,Engine_Type type,int sche_wait_time,int max_event_size, int threadnum)
{
    return std::make_shared<HttpServerConf>(backlog,max_rbuffer_len,type,sche_wait_time,max_event_size,threadnum);
}

//https

galay::HttpSSLServerConf::ptr galay::Config_Factory::create_https_server_config(Engine_Type type,int sche_wait_time, int threadnum)
{
    return std::make_shared<HttpSSLServerConf>(DEFAULT_BACKLOG,DEFAULT_RECV_LENGTH,type,sche_wait_time,DEFAULT_EVENT_SIZE,threadnum);
}

galay::HttpSSLServerConf::ptr galay::Config_Factory::create_https_server_config(HttpSSLServerConf &&config)
{
    return std::make_shared<HttpSSLServerConf>(config);
}

galay::HttpSSLServerConf::ptr galay::Config_Factory::create_https_server_config(uint32_t backlog,uint32_t max_rbuffer_len,Engine_Type type,int sche_wait_time,int max_event_size, int threadnum)
{
    return std::make_shared<HttpSSLServerConf>(backlog,max_rbuffer_len,type,sche_wait_time,max_event_size,threadnum);
}




//server
galay::Tcp_Server<galay::Protocol::Http1_1_Request,galay::Protocol::Http1_1_Response>::uptr galay::Server_Factory::create_http_server(HttpServerConf::ptr config)
{
    return std::make_unique<Tcp_Server<Protocol::Http1_1_Request,Protocol::Http1_1_Response>>(config);
}

galay::Tcp_SSL_Server<galay::Protocol::Http1_1_Request,galay::Protocol::Http1_1_Response>::uptr galay::Server_Factory::create_https_server(HttpSSLServerConf::ptr config)
{
    return std::make_unique<Tcp_SSL_Server<Protocol::Http1_1_Request,Protocol::Http1_1_Response>>(config);
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
galay::Tcp_Client::uptr galay::Client_Factory::create_tcp_client(Scheduler_Base::wptr scheduler)
{
    return std::make_unique<Tcp_Client>(scheduler);
}

galay::Tcp_SSL_Client::uptr galay::Client_Factory::create_tcp_ssl_client(Scheduler_Base::ptr scheduler, long ssl_min_version , long ssl_max_version)
{
    return std::make_unique<Tcp_SSL_Client>(scheduler,ssl_min_version,ssl_max_version);
}


galay::Tcp_Request_Client::uptr galay::Client_Factory::create_http_client(Scheduler_Base::wptr scheduler)
{
    return std::make_unique<Tcp_Request_Client>(scheduler);
}

galay::Tcp_SSL_Request_Client::uptr galay::Client_Factory::create_https_client(Scheduler_Base::wptr scheduler, long ssl_min_version , long ssl_max_version)
{
    return std::make_unique<Tcp_SSL_Request_Client>(scheduler,ssl_min_version,ssl_max_version);
}

galay::Tcp_Request_Client::uptr galay::Client_Factory::create_smtp_client(Scheduler_Base::wptr scheduler)
{
    return std::make_unique<Tcp_Request_Client>(scheduler);
}

galay::Tcp_SSL_Request_Client::uptr galay::Client_Factory::create_smtps_client(Scheduler_Base::wptr scheduler, long ssl_min_version , long ssl_max_version)
{
    return std::make_unique<Tcp_SSL_Request_Client>(scheduler,ssl_min_version,ssl_max_version);
}

galay::Tcp_Request_Client::uptr galay::Client_Factory::create_tcp_self_define_client(Scheduler_Base::wptr scheduler)
{
    return std::make_unique<Tcp_Request_Client>(scheduler);
}

galay::Tcp_SSL_Request_Client::uptr galay::Client_Factory::create_tcp_ssl_self_define_client(Scheduler_Base::wptr scheduler, long ssl_min_version , long ssl_max_version)
{
    return std::make_unique<Tcp_SSL_Request_Client>(scheduler,ssl_min_version,ssl_max_version);
}

//udp client
galay::Udp_Client::uptr galay::Client_Factory::create_udp_client(Scheduler_Base::wptr scheduler)
{
    return std::make_unique<Udp_Client>(scheduler);
}

galay::Udp_Request_Client::uptr galay::Client_Factory::create_dns_client(Scheduler_Base::wptr scheduler)
{
    return std::make_unique<Udp_Request_Client>(scheduler);
}

//pool
//threadpool
galay::ThreadPool::ptr galay::Pool_Factory::create_threadpool(int num)
{
    return std::make_shared<ThreadPool>(num);
}