#ifndef GALAY_FACTORY_H
#define GALAY_FACTORY_H
#include "builder.h"
#include <memory>

namespace galay::server
{
    class HttpServerBuilder;
    class HttpRouter;
    class TcpServer;
}

namespace galay{

    class Factory{
    public:
        using ptr = std::shared_ptr<Factory>;
        using uptr = std::unique_ptr<Factory>;
        using wptr = std::weak_ptr<Factory>;
        virtual ~Factory() = default;
    };
    
    class RouterFactory{
    public:
        static std::shared_ptr<server::HttpRouter> CreateHttpRouter(); 
    };

    class ServerBuilderFactory{
    public:
        static std::shared_ptr<server::HttpServerBuilder> CreateHttpServerBuilder(\
            int port,std::shared_ptr<server::HttpRouter> router,\
            server::PollerType type = server::PollerType::kEpollPoller ,uint16_t thread_num = DEFAULT_THREAD_NUM);
        static std::shared_ptr<server::HttpServerBuilder> CreateHttpsServerBuilder(\
            const std::string& key_file,const std::string& cert_file, int port,\
            std::shared_ptr<server::HttpRouter> router,server::PollerType type = server::PollerType::kEpollPoller\
            ,uint16_t thread_num = DEFAULT_THREAD_NUM);
    };

    class ServerFactory{
    public:
        static std::shared_ptr<server::TcpServer> CreateHttpServer(std::shared_ptr<server::HttpServerBuilder> builder);
        static std::shared_ptr<server::TcpServer> CreateHttpsServer(std::shared_ptr<server::HttpServerBuilder> builder);
    };
}


#endif