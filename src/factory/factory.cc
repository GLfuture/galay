#include "factory.h"

//tcp
galay::Tcp_Server_Config::ptr galay::Config_Factory::create_tcp_server_config(int port)
{
    return std::make_shared<Tcp_Server_Config>(Tcp_Server_Config(port,DEFAULT_BACKLOG, DEFAULT_ENGINE));
}


galay::Tcp_Server_Config::ptr galay::Config_Factory::create_tcp_server_config(Tcp_Server_Config &&config)
{
    return std::make_shared<Tcp_Server_Config>(config);
}

galay::Tcp_Server_Config::ptr galay::Config_Factory::create_tcp_server_config(uint16_t port,uint32_t backlog,IO_ENGINE engine)
{
    return std::make_shared<Tcp_Server_Config>(port,backlog,engine);
}

//tcp ssl
galay::Tcp_SSL_Server_Config::ptr galay::Config_Factory::create_tcp_ssl_server_config(int port,long ssl_min_version,long ssl_max_version
            ,const char* cert_filepath,const char* key_filepath)
{
    return std::make_shared<Tcp_SSL_Server_Config>(Tcp_SSL_Server_Config(port,DEFAULT_BACKLOG,DEFAULT_ENGINE
        ,DEFAULT_MAX_SSL_ACCEPT_RETRY,ssl_min_version,ssl_max_version,cert_filepath,key_filepath));
}


galay::Tcp_SSL_Server_Config::ptr galay::Config_Factory::create_tcp_ssl_server_config(Tcp_SSL_Server_Config &&config)
{
    return std::make_shared<Tcp_SSL_Server_Config>(config);
}

galay::Tcp_SSL_Server_Config::ptr galay::Config_Factory::create_tcp_ssl_server_config(uint16_t port,uint32_t backlog,IO_ENGINE engine 
            , long ssl_min_version , long ssl_max_version, uint32_t ssl_max_accept_retry , const char* cert_filepath, const char* key_filepath)
{
    return std::make_shared<Tcp_SSL_Server_Config>(port,backlog,engine,ssl_min_version,ssl_max_version,ssl_max_accept_retry ,cert_filepath,key_filepath);
}


//http
galay::Http_Server_Config::ptr galay::Config_Factory::create_http_server_config(int port)
{
    return std::make_shared<Http_Server_Config>(port,DEFAULT_BACKLOG,DEFAULT_ENGINE);
}


galay::Http_Server_Config::ptr galay::Config_Factory::create_http_server_config(Http_Server_Config &&config)
{
    return std::make_shared<Http_Server_Config>(config);
}

galay::Http_Server_Config::ptr galay::Config_Factory::create_http_server_config(uint16_t port,uint32_t backlog,IO_ENGINE engine)
{
    return std::make_shared<Http_Server_Config>(port,backlog,engine);
}

//https

galay::Https_Server_Config::ptr galay::Config_Factory::create_https_server_config(int port,long ssl_min_version,long ssl_max_version
            , const char* cert_filepath,const char* key_filepath)
{
    return std::make_shared<Https_Server_Config>(port,DEFAULT_BACKLOG,DEFAULT_ENGINE,ssl_min_version,ssl_max_version
        ,DEFAULT_MAX_SSL_ACCEPT_RETRY,cert_filepath,key_filepath);
}

galay::Https_Server_Config::ptr galay::Config_Factory::create_https_server_config(Https_Server_Config &&config)
{
    return std::make_shared<Https_Server_Config>(config);
}

galay::Https_Server_Config::ptr galay::Config_Factory::create_https_server_config(uint16_t port,uint32_t backlog,IO_ENGINE engine , long ssl_min_version , long ssl_max_version
            , uint32_t ssl_accept_max_retry ,const char* cert_filepath,const char* key_filepath)
{
    return std::make_shared<Https_Server_Config>(port,backlog,engine,ssl_min_version,ssl_max_version,ssl_accept_max_retry,cert_filepath,key_filepath);
}




//server
galay::Tcp_Server<galay::Tcp_Request,galay::Tcp_Response>::ptr galay::Server_Factory::create_tcp_server(Tcp_Server_Config::ptr config)
{
    return std::make_shared<Tcp_Server<galay::Tcp_Request,galay::Tcp_Response>>(config);
}

galay::Tcp_SSL_Server<galay::Tcp_Request,galay::Tcp_Response>::ptr galay::Server_Factory::create_tcp_ssl_server(Tcp_SSL_Server_Config::ptr config)
{
    return std::make_shared<Tcp_SSL_Server<galay::Tcp_Request,galay::Tcp_Response>>(config);
}

galay::Http_Server<galay::Http_Request,galay::Http_Response>::ptr galay::Server_Factory::create_http_server(Http_Server_Config::ptr config)
{
    return std::make_shared<Http_Server<galay::Http_Request,galay::Http_Response>>(config);
}

galay::Https_Server<galay::Http_Request,galay::Http_Response>::ptr galay::Server_Factory::create_https_server(Https_Server_Config::ptr config)
{
    return std::make_shared<Https_Server<galay::Http_Request,galay::Http_Response>>(config);
}

//scheduler
galay::IO_Scheduler<galay::Tcp_Request,galay::Tcp_Response>::ptr galay::Scheduler_Factory::create_tcp_scheduler(IO_ENGINE engine_type,int event_num,int time_out)
{
    return std::make_shared<IO_Scheduler<galay::Tcp_Request,galay::Tcp_Response>>(engine_type,event_num,time_out);
}


//client
galay::Tcp_Client<galay::Tcp_Request,galay::Tcp_Response>::ptr galay::Client_Factory::create_tcp_client(IO_Scheduler<Tcp_Request,Tcp_Response>::ptr scheduler)
{
    return std::make_shared<Tcp_Client<galay::Tcp_Request,galay::Tcp_Response>>(scheduler);
}