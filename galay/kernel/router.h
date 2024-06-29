#ifndef GALAY_ROUTER_H
#define GALAY_ROUTER_H

#include "../common/coroutine.h"

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
            void Get(const std::string &path, std::function<common::GY_TcpCoroutine<common::CoroutineStatus>(std::weak_ptr<GY_HttpController>)> func);
            void Post(const std::string &path, std::function<common::GY_TcpCoroutine<common::CoroutineStatus>(std::weak_ptr<GY_HttpController>)> func);

        protected:
            void RegisterRouter(const std::string &method, const std::string &path, std::function<common::GY_TcpCoroutine<common::CoroutineStatus>(std::weak_ptr<GY_HttpController>)> func);
            common::GY_TcpCoroutine<common::CoroutineStatus> RouteHttp(std::weak_ptr<GY_Controller> ctrl);

        private:
            std::unordered_map<std::string, std::unordered_map<std::string, std::function<common::GY_TcpCoroutine<common::CoroutineStatus>(std::weak_ptr<GY_HttpController>)>>> m_routes;
            common::GY_TcpCoroutine<common::CoroutineStatus> m_coroBusiness;
        };
    }
}

#endif