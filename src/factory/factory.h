#ifndef GALAY_FACTORY_H
#define GALAY_FACTORY_H

#include "../config/config.h"

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
        //default
        static Tcp_Server_Config::ptr create_tcp_server_config(int port);
        static Tcp_Server_Config::ptr create_tcp_server_config(Tcp_Server_Config &&config);
        static Http_Server_Config::ptr create_http_server_config(Http_Server_Config &&config);
    };
}


#endif