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
            void Get(const std::string &path, std::function<common::GY_NetCoroutine<common::CoroutineStatus>(std::weak_ptr<GY_HttpController>)> func);
            void Post(const std::string &path, std::function<common::GY_NetCoroutine<common::CoroutineStatus>(std::weak_ptr<GY_HttpController>)> func);
            void Options(const std::string &path, std::function<common::GY_NetCoroutine<common::CoroutineStatus>(std::weak_ptr<GY_HttpController>)> func);
            void Put(const std::string &path, std::function<common::GY_NetCoroutine<common::CoroutineStatus>(std::weak_ptr<GY_HttpController>)> func);
            void Delete(const std::string &path, std::function<common::GY_NetCoroutine<common::CoroutineStatus>(std::weak_ptr<GY_HttpController>)> func);
            void Patch(const std::string &path, std::function<common::GY_NetCoroutine<common::CoroutineStatus>(std::weak_ptr<GY_HttpController>)> func);
            void Head(const std::string &path, std::function<common::GY_NetCoroutine<common::CoroutineStatus>(std::weak_ptr<GY_HttpController>)> func);
            void Trace(const std::string &path, std::function<common::GY_NetCoroutine<common::CoroutineStatus>(std::weak_ptr<GY_HttpController>)> func);
            void Connect(const std::string &path, std::function<common::GY_NetCoroutine<common::CoroutineStatus>(std::weak_ptr<GY_HttpController>)> func);
        protected:
            void RegisterRouter(const std::string &method, const std::string &path, std::function<common::GY_NetCoroutine<common::CoroutineStatus>(std::weak_ptr<GY_HttpController>)> func);
            common::GY_NetCoroutine<common::CoroutineStatus> RouteHttp(std::weak_ptr<GY_Controller> ctrl);

        private:
            std::unordered_map<std::string, std::unordered_map<std::string, std::function<common::GY_NetCoroutine<common::CoroutineStatus>(std::weak_ptr<GY_HttpController>)>>> m_routes;
            uint64_t m_coId;
        };
    }
}

#endif