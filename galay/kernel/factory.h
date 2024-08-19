#ifndef GALAY_FACTORY_H
#define GALAY_FACTORY_H
#include "../common/base.h"
#include <memory>


namespace galay{

    class GY_Factory{
    public:
        using ptr = std::shared_ptr<GY_Factory>;
        using uptr = std::unique_ptr<GY_Factory>;
        using wptr = std::weak_ptr<GY_Factory>;

        virtual ~GY_Factory() = default;
    };

    namespace server
    {
        class GY_HttpServerBuilder;
        class GY_HttpRouter;
        class GY_TcpServer;
    }
    
    class GY_RouterFactory{
    public:
        static std::shared_ptr<server::GY_HttpRouter> CreateHttpRouter(); 
    };

    class GY_ServerBuilderFactory{
    public:
        static std::shared_ptr<server::GY_HttpServerBuilder> CreateHttpServerBuilder(\
            int port,std::shared_ptr<server::GY_HttpRouter> router,\
            common::SchedulerType type = common::SchedulerType::kEpollScheduler,uint16_t thread_num = DEFAULT_THREAD_NUM);
        static std::shared_ptr<server::GY_HttpServerBuilder> CreateHttpsServerBuilder(\
            const std::string& key_file,const std::string& cert_file, int port,\
            std::shared_ptr<server::GY_HttpRouter> router,common::SchedulerType type = common::SchedulerType::kEpollScheduler\
            ,uint16_t thread_num = DEFAULT_THREAD_NUM);
    };

    class GY_ServerFactory{
    public:
        static std::shared_ptr<server::GY_TcpServer> CreateHttpServer(std::shared_ptr<server::GY_HttpServerBuilder> builder);
        static std::shared_ptr<server::GY_TcpServer> CreateHttpsServer(std::shared_ptr<server::GY_HttpServerBuilder> builder);
    };
}


#endif