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
        ,ssl_min_version,ssl_max_version,cert_filepath,key_filepath));
}


galay::Tcp_SSL_Server_Config::ptr galay::Config_Factory::create_tcp_ssl_server_config(Tcp_SSL_Server_Config &&config)
{
    return std::make_shared<Tcp_SSL_Server_Config>(config);
}

galay::Tcp_SSL_Server_Config::ptr galay::Config_Factory::create_tcp_ssl_server_config(uint16_t port,uint32_t backlog,IO_ENGINE engine 
            , long ssl_min_version , long ssl_max_version , const char* cert_filepath, const char* key_filepath)
{
    return std::make_shared<Tcp_SSL_Server_Config>(port,backlog,engine,ssl_min_version,ssl_max_version,cert_filepath,key_filepath);
}


//http
galay::Http_Server_Config::ptr galay::Config_Factory::create_http_server_config(Http_Server_Config &&config)
{
    return std::make_shared<Http_Server_Config>(config);
}