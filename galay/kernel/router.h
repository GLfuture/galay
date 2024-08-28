#ifndef GALAY_ROUTER_H
#define GALAY_ROUTER_H

#include "../common/base.h"
#include "../protocol/http.h"

namespace galay::coroutine
{
    class NetCoroutine;
}

namespace galay::server
{
    class Controller;
    class HttpController;

    class HttpRouter
    {
    public:
        HttpRouter();
        void Get(const std::string &path, std::function<coroutine::NetCoroutine(std::shared_ptr<HttpController>)> func);
        void Post(const std::string &path, std::function<coroutine::NetCoroutine(std::shared_ptr<HttpController>)> func);
        void Options(const std::string &path, std::function<coroutine::NetCoroutine(std::shared_ptr<HttpController>)> func);
        void Put(const std::string &path, std::function<coroutine::NetCoroutine(std::shared_ptr<HttpController>)> func);
        void Delete(const std::string &path, std::function<coroutine::NetCoroutine(std::shared_ptr<HttpController>)> func);
        void Patch(const std::string &path, std::function<coroutine::NetCoroutine(std::shared_ptr<HttpController>)> func);
        void Head(const std::string &path, std::function<coroutine::NetCoroutine(std::shared_ptr<HttpController>)> func);
        void Trace(const std::string &path, std::function<coroutine::NetCoroutine(std::shared_ptr<HttpController>)> func);
        void Connect(const std::string &path, std::function<coroutine::NetCoroutine(std::shared_ptr<HttpController>)> func);
        //Parse Error
        void RegisterWrongHandle(std::function<coroutine::NetCoroutine(std::shared_ptr<HttpController>)> handle);
    protected:
        void RegisterRouteHandle(const std::string &method, const std::string &path, std::function<coroutine::NetCoroutine(std::shared_ptr<HttpController>)> func);
        void RouteRightHttp(std::shared_ptr<Controller> ctrl);
        bool RouteSuccess(protocol::http::HttpRequest::ptr request);
    private:
        uint64_t m_coId;
        std::shared_ptr<HttpController> m_httpCtrl;
        std::unordered_map<std::string, std::unordered_map<std::string, std::function<void(std::shared_ptr<HttpController>)>>> m_routes;
    };
}


#endif