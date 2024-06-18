#ifndef GALAY_ROUTER_H
#define GALAY_ROUTER_H

#include "coroutine.h"


namespace galay
{
    class GY_Controller;
    class GY_HttpController;

    class GY_HttpRouter
    {
        friend class GY_HttpServerBuilder;
    public:
        void Get(const std::string& path,std::function<GY_TcpCoroutine<galay::CoroutineStatus>(std::weak_ptr<GY_HttpController>)> func);
        void Post(const std::string& path,std::function<GY_TcpCoroutine<galay::CoroutineStatus>(std::weak_ptr<GY_HttpController>)> func);
    protected:
        void RegisterRouter(const std::string& method,const std::string& path,std::function<GY_TcpCoroutine<galay::CoroutineStatus>(std::weak_ptr<GY_HttpController>)> func);
        galay::GY_TcpCoroutine<galay::CoroutineStatus> RouteHttp(std::weak_ptr<GY_Controller> ctrl);
        
    private:
        std::unordered_map<std::string,std::unordered_map<std::string,std::function<GY_TcpCoroutine<galay::CoroutineStatus>(std::weak_ptr<GY_HttpController>)>>> m_routes;
    };
}



#endif