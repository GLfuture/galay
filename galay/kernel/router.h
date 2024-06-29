#ifndef GALAY_ROUTER_H
#define GALAY_ROUTER_H

#include "coroutine.h"

namespace galay
{
    namespace kernel
    {
        class GY_Controller;
        class GY_HttpController;

        class GY_HttpRouter
        {
            friend class GY_HttpServerBuilder;

        public:
            void Get(const std::string &path, std::function<GY_TcpCoroutine<CoroutineStatus>(std::weak_ptr<GY_HttpController>)> func);
            void Post(const std::string &path, std::function<GY_TcpCoroutine<CoroutineStatus>(std::weak_ptr<GY_HttpController>)> func);

        protected:
            void RegisterRouter(const std::string &method, const std::string &path, std::function<GY_TcpCoroutine<CoroutineStatus>(std::weak_ptr<GY_HttpController>)> func);
            GY_TcpCoroutine<CoroutineStatus> RouteHttp(std::weak_ptr<GY_Controller> ctrl);

        private:
            std::unordered_map<std::string, std::unordered_map<std::string, std::function<GY_TcpCoroutine<CoroutineStatus>(std::weak_ptr<GY_HttpController>)>>> m_routes;
            GY_TcpCoroutine<CoroutineStatus> m_coroBusiness;
        };
    }
}

#endif