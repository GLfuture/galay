#include "router.h"
#include "controller.h"
#include "poller.h"
#include "../common/coroutine.h"


namespace galay::server
{

HttpRouter::HttpRouter()
{
    poller::SetRightHandle([this](std::shared_ptr<Controller> ctrl){
        this->RouteRightHttp(ctrl);
    });
}

void 
HttpRouter::Get(const std::string &path, std::function<coroutine::NetCoroutine(std::shared_ptr<HttpController>)> func)
{
    RegisterRouteHandle("GET", path, func);
}

void 
HttpRouter::Post(const std::string &path, std::function<coroutine::NetCoroutine(std::shared_ptr<HttpController>)> func)
{
    RegisterRouteHandle("POST", path, func);
}

void 
HttpRouter::Options(const std::string &path, std::function<coroutine::NetCoroutine(std::shared_ptr<HttpController>)> func)
{
    RegisterRouteHandle("OPTIONS", path, func);
}

void 
HttpRouter::Put(const std::string &path, std::function<coroutine::NetCoroutine(std::shared_ptr<HttpController>)> func)
{
    RegisterRouteHandle("PUT", path, func);
}

void 
HttpRouter::Delete(const std::string &path, std::function<coroutine::NetCoroutine(std::shared_ptr<HttpController>)> func)
{
    RegisterRouteHandle("DELETE", path, func);
}

void 
HttpRouter::Patch(const std::string &path, std::function<coroutine::NetCoroutine(std::shared_ptr<HttpController>)> func)
{
    RegisterRouteHandle("PATCH", path, func);
}

void 
HttpRouter::Head(const std::string &path, std::function<coroutine::NetCoroutine(std::shared_ptr<HttpController>)> func)
{
    RegisterRouteHandle("HEAD", path, func);
}

void 
HttpRouter::Trace(const std::string &path, std::function<coroutine::NetCoroutine(std::shared_ptr<HttpController>)> func)
{
    RegisterRouteHandle("TRACE", path, func);
}

void 
HttpRouter::Connect(const std::string &path, std::function<coroutine::NetCoroutine(std::shared_ptr<HttpController>)> func)
{
    RegisterRouteHandle("CONNECT", path, func);
}

void 
HttpRouter::RegisterWrongHandle(std::function<coroutine::NetCoroutine(std::shared_ptr<HttpController>)> handle)
{
    poller::SetWrongHandle([this, handle](std::shared_ptr<Controller> ctrl){
        this->m_httpCtrl = std::make_shared<HttpController>(ctrl);
        handle(this->m_httpCtrl);
    });
}


void 
HttpRouter::RegisterRouteHandle(const std::string &mehtod, const std::string &path, std::function<coroutine::NetCoroutine(std::shared_ptr<HttpController>)> handle)
{
    this->m_routes[mehtod][path] = [handle](std::shared_ptr<HttpController> ctrl){
        handle(ctrl);
    };
}

void
HttpRouter::RouteRightHttp(std::shared_ptr<Controller> ctrl)
{
    auto request = std::dynamic_pointer_cast<protocol::http::HttpRequest>(ctrl->GetRequest());
    m_httpCtrl = std::make_shared<HttpController>(ctrl);
    if (RouteSuccess(request))
    {
        auto func = m_routes[request->Header()->Method()][request->Header()->Uri()];
        func(m_httpCtrl);
    }
    else 
    {
        m_httpCtrl->Close();
    }
}

bool
HttpRouter::RouteSuccess(galay::protocol::http::HttpRequest::ptr request)
{
    auto method_routers = m_routes.find(request->Header()->Method());
    if (method_routers != m_routes.end())
    {
        auto router = method_routers->second.find(request->Header()->Uri());
        if (router == method_routers->second.end())
        {
            auto response = std::make_shared<protocol::http::HttpResponse>();
            response->Header()->Code() = protocol::http::NotFound_404;
            response->Header()->Version() = "1.1";
            response->Header()->HeaderPairs().AddHeaderPair("Connection", "close");
            response->Header()->HeaderPairs().AddHeaderPair("Content-Type", "text/html");
            response->Header()->HeaderPairs().AddHeaderPair("Server-Framwork", "galay");
            response->Body() = html::Html404NotFound;
            m_httpCtrl->Send(response->EncodePdu());
            return false;
        }
    }
    else
    {
        auto response = std::make_shared<protocol::http::HttpResponse>();
        response->Header()->Code() = protocol::http::MethodNotAllowed_405;
        response->Header()->Version() = "1.1";
        response->Header()->HeaderPairs().AddHeaderPair("Connection", "close");
        response->Header()->HeaderPairs().AddHeaderPair("Content-Type", "text/html");
        response->Header()->HeaderPairs().AddHeaderPair("Server-Framwork", "galay");
        response->Body() = html::Html405MethodNotAllowed;
        m_httpCtrl->Send(response->EncodePdu());
        return false;
    }
    return true;
}
}