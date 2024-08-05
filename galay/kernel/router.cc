#include "router.h"
#include "controller.h"
#include "../common/coroutine.h"


void 
galay::kernel::GY_HttpRouter::Get(const std::string &path, std::function<common::GY_NetCoroutine(std::shared_ptr<GY_HttpController>)> func)
{
    RegisterRouter("GET", path, func);
}

void 
galay::kernel::GY_HttpRouter::Post(const std::string &path, std::function<common::GY_NetCoroutine(std::shared_ptr<GY_HttpController>)> func)
{
    RegisterRouter("POST", path, func);
}

void 
galay::kernel::GY_HttpRouter::Options(const std::string &path, std::function<common::GY_NetCoroutine(std::shared_ptr<GY_HttpController>)> func)
{
    RegisterRouter("OPTIONS", path, func);
}

void 
galay::kernel::GY_HttpRouter::Put(const std::string &path, std::function<common::GY_NetCoroutine(std::shared_ptr<GY_HttpController>)> func)
{
    RegisterRouter("PUT", path, func);
}

void 
galay::kernel::GY_HttpRouter::Delete(const std::string &path, std::function<common::GY_NetCoroutine(std::shared_ptr<GY_HttpController>)> func)
{
    RegisterRouter("DELETE", path, func);
}

void 
galay::kernel::GY_HttpRouter::Patch(const std::string &path, std::function<common::GY_NetCoroutine(std::shared_ptr<GY_HttpController>)> func)
{
    RegisterRouter("PATCH", path, func);
}

void 
galay::kernel::GY_HttpRouter::Head(const std::string &path, std::function<common::GY_NetCoroutine(std::shared_ptr<GY_HttpController>)> func)
{
    RegisterRouter("HEAD", path, func);
}

void 
galay::kernel::GY_HttpRouter::Trace(const std::string &path, std::function<common::GY_NetCoroutine(std::shared_ptr<GY_HttpController>)> func)
{
    RegisterRouter("TRACE", path, func);
}

void 
galay::kernel::GY_HttpRouter::Connect(const std::string &path, std::function<common::GY_NetCoroutine(std::shared_ptr<GY_HttpController>)> func)
{
    RegisterRouter("CONNECT", path, func);
}


void 
galay::kernel::GY_HttpRouter::RegisterRouter(const std::string &mehtod, const std::string &path, std::function<common::GY_NetCoroutine(std::shared_ptr<GY_HttpController>)> func)
{
    this->m_routes[mehtod][path] = [func](std::shared_ptr<GY_HttpController> ctrl){
        func(ctrl);
    };
}


void
galay::kernel::GY_HttpRouter::RouteHttp(std::shared_ptr<GY_Controller> ctrl)
{
    auto request = std::dynamic_pointer_cast<protocol::http::HttpRequest>(ctrl->GetRequest());
    m_httpCtrl = std::make_shared<galay::kernel::GY_HttpController>(ctrl);
    if (RouteSuccess(request))
    {
        auto func = m_routes[request->Header()->Method()][request->Header()->Uri()];
        func(m_httpCtrl);
    }
}

bool
galay::kernel::GY_HttpRouter::RouteSuccess(galay::protocol::http::HttpRequest::ptr request)
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
            response->Header()->Headers()["Connection"] = "close";
            response->Header()->Headers()["Content-Type"] = "text/html";
            response->Header()->Headers()["Server"] = "galay server";
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
        response->Header()->Headers()["Connection"] = "close";
        response->Header()->Headers()["Content-Type"] = "text/html";
        response->Header()->Headers()["Server"] = "galay server";
        response->Body() = html::Html405MethodNotAllowed;
        m_httpCtrl->Send(response->EncodePdu());
        return false;
    }
    return true;
}