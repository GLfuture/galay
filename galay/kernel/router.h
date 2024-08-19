#ifndef GALAY_ROUTER_H
#define GALAY_ROUTER_H

#include "../common/base.h"
#include "../protocol/http.h"

namespace galay
{
    namespace coroutine
    {
        class GY_NetCoroutine;
    }
    namespace server
    {
        class GY_Controller;
        class GY_HttpController;

        class GY_HttpRouter
        {
            friend class GY_HttpServerBuilder;

        public:
            void Get(const std::string &path, std::function<coroutine::GY_NetCoroutine(std::shared_ptr<GY_HttpController>)> func);
            void Post(const std::string &path, std::function<coroutine::GY_NetCoroutine(std::shared_ptr<GY_HttpController>)> func);
            void Options(const std::string &path, std::function<coroutine::GY_NetCoroutine(std::shared_ptr<GY_HttpController>)> func);
            void Put(const std::string &path, std::function<coroutine::GY_NetCoroutine(std::shared_ptr<GY_HttpController>)> func);
            void Delete(const std::string &path, std::function<coroutine::GY_NetCoroutine(std::shared_ptr<GY_HttpController>)> func);
            void Patch(const std::string &path, std::function<coroutine::GY_NetCoroutine(std::shared_ptr<GY_HttpController>)> func);
            void Head(const std::string &path, std::function<coroutine::GY_NetCoroutine(std::shared_ptr<GY_HttpController>)> func);
            void Trace(const std::string &path, std::function<coroutine::GY_NetCoroutine(std::shared_ptr<GY_HttpController>)> func);
            void Connect(const std::string &path, std::function<coroutine::GY_NetCoroutine(std::shared_ptr<GY_HttpController>)> func);
            //Parse Error
            void RegisterWrongHandle(std::function<coroutine::GY_NetCoroutine(std::shared_ptr<GY_HttpController>)> func);
        protected:
            void RegisterRouteHandle(const std::string &method, const std::string &path, std::function<coroutine::GY_NetCoroutine(std::shared_ptr<GY_HttpController>)> func);
            void RouteRightHttp(std::shared_ptr<GY_Controller> ctrl);
            void RouteWrongHttp(std::shared_ptr<GY_Controller> ctrl);
            bool RouteSuccess(protocol::http::HttpRequest::ptr request);
        private:
            uint64_t m_coId;
            std::shared_ptr<GY_HttpController> m_httpCtrl;
            std::function<void(std::shared_ptr<GY_HttpController>)> m_wrongHandle;
            std::unordered_map<std::string, std::unordered_map<std::string, std::function<void(std::shared_ptr<GY_HttpController>)>>> m_routes;
        };
    }
}

#endif