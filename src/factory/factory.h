#ifndef GALAY_FACTORY_H
#define GALAY_FACTORY_H

#include "../config/config.h"
#include "../server/tcpserver.hpp"
#include "../server/httpserver.hpp"
#include "../protocol/tcp.h"
#include "../protocol/http.h"

namespace galay
{
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
        static Tcp_Server_Config::ptr create_tcp_server_config(uint16_t port,uint32_t backlog,IO_ENGINE engine);

        //tcp ssl
        static Tcp_SSL_Server_Config::ptr create_tcp_ssl_server_config(int port,long ssl_min_version,long ssl_max_version
            , const char* cert_filepath,const char* key_filepath);
        static Tcp_SSL_Server_Config::ptr create_tcp_ssl_server_config(Tcp_SSL_Server_Config &&config);
        static Tcp_SSL_Server_Config::ptr create_tcp_ssl_server_config(uint16_t port,uint32_t backlog,IO_ENGINE engine 
            , long ssl_min_version , long ssl_max_version ,uint32_t ssl_max_accept_retry,const char* cert_filepath, const char* key_filepath);

        //http
        static Http_Server_Config::ptr create_http_server_config(int port);
        static Http_Server_Config::ptr create_http_server_config(Http_Server_Config &&config);
        static Http_Server_Config::ptr create_http_server_config(uint16_t port,uint32_t backlog,IO_ENGINE engine);


        //https
        static Https_Server_Config::ptr create_https_server_config(int port,long ssl_min_version,long ssl_max_version
            , const char* cert_filepath,const char* key_filepath);
        static Https_Server_Config::ptr create_https_server_config(Https_Server_Config &&config);
        static Https_Server_Config::ptr create_https_server_config(uint16_t port,uint32_t backlog,IO_ENGINE engine 
            , long ssl_min_version , long ssl_max_version, uint32_t ssl_accept_max_retry ,const char* cert_filepath,const char* key_filepath);
    };

    class Server_Factory:public Factory_Base
    {
    public:
        using ptr = std::shared_ptr<Server_Factory>;
        //tcp
        static Tcp_Server<Tcp_Request,Tcp_Response>::ptr create_tcp_server(Tcp_Server_Config::ptr config);
        //tcp ssl
        static Tcp_SSL_Server<Tcp_Request,Tcp_Response>::ptr create_tcp_ssl_server(Tcp_SSL_Server_Config::ptr config);
        //http
        static Http_Server<Http_Request,Http_Response>::ptr create_http_server(Http_Server_Config::ptr config);
        //https
        static Https_Server<Http_Request,Http_Response>::ptr create_https_server(Https_Server_Config::ptr config);
    };

}


#endif