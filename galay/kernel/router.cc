#include "router.h"
#include "controller.h"


void 
galay::kernel::GY_HttpRouter::Get(const std::string &path, std::function<common::GY_NetCoroutine<common::CoroutineStatus>(std::weak_ptr<GY_HttpController>)> func)
{
    RegisterRouter("GET", path, func);
}

void 
galay::kernel::GY_HttpRouter::Post(const std::string &path, std::function<common::GY_NetCoroutine<common::CoroutineStatus>(std::weak_ptr<GY_HttpController>)> func)
{
    RegisterRouter("POST", path, func);
}

void 
galay::kernel::GY_HttpRouter::Options(const std::string &path, std::function<common::GY_NetCoroutine<common::CoroutineStatus>(std::weak_ptr<GY_HttpController>)> func)
{
    RegisterRouter("OPTIONS", path, func);
}

void 
galay::kernel::GY_HttpRouter::Put(const std::string &path, std::function<common::GY_NetCoroutine<common::CoroutineStatus>(std::weak_ptr<GY_HttpController>)> func)
{
    RegisterRouter("PUT", path, func);
}

void 
galay::kernel::GY_HttpRouter::Delete(const std::string &path, std::function<common::GY_NetCoroutine<common::CoroutineStatus>(std::weak_ptr<GY_HttpController>)> func)
{
    RegisterRouter("DELETE", path, func);
}

void 
galay::kernel::GY_HttpRouter::Patch(const std::string &path, std::function<common::GY_NetCoroutine<common::CoroutineStatus>(std::weak_ptr<GY_HttpController>)> func)
{
    RegisterRouter("PATCH", path, func);
}

void 
galay::kernel::GY_HttpRouter::Head(const std::string &path, std::function<common::GY_NetCoroutine<common::CoroutineStatus>(std::weak_ptr<GY_HttpController>)> func)
{
    RegisterRouter("HEAD", path, func);
}

void 
galay::kernel::GY_HttpRouter::Trace(const std::string &path, std::function<common::GY_NetCoroutine<common::CoroutineStatus>(std::weak_ptr<GY_HttpController>)> func)
{
    RegisterRouter("TRACE", path, func);
}

void 
galay::kernel::GY_HttpRouter::Connect(const std::string &path, std::function<common::GY_NetCoroutine<common::CoroutineStatus>(std::weak_ptr<GY_HttpController>)> func)
{
    RegisterRouter("CONNECT", path, func);
}


void 
galay::kernel::GY_HttpRouter::RegisterRouter(const std::string &mehtod, const std::string &path, std::function<common::GY_NetCoroutine<common::CoroutineStatus>(std::weak_ptr<GY_HttpController>)> func)
{
    this->m_routes[mehtod][path] = func;
}


galay::common::GY_NetCoroutine<galay::common::CoroutineStatus>
galay::kernel::GY_HttpRouter::RouteHttp(std::weak_ptr<GY_Controller> ctrl)
{
    auto request = std::dynamic_pointer_cast<protocol::http::HttpRequest>(ctrl.lock()->GetRequest());
    common::WaitGroup group(ctrl.lock()->GetCoPool());
    m_httpCtrl = std::make_shared<galay::kernel::GY_HttpController>(ctrl);
    m_httpCtrl->SetWaitGroup(&group);
    auto method_routers = m_routes.find(request->Header()->Method());
    if (method_routers != m_routes.end())
    {
        auto router = method_routers->second.find(request->Header()->Uri());
        if (router != method_routers->second.end())
        {
            group.Add(1);
            m_httpCtrl->SetRequest(request);
            auto co = router->second(m_httpCtrl);
            if(co.IsCoroutine() && co.GetStatus() != common::kCoroutineFinished) ctrl.lock()->GetCoPool().lock()->AddCoroutine(std::move(co)) ;
        }
        else
        {
            auto response = std::make_shared<protocol::http::HttpResponse>();
            response->Header()->Code() = protocol::http::NotFound_404;
            response->Header()->Version() = "1.1";
            response->Header()->Headers()["Connection"] = "close";
            response->Header()->Headers()["Content-Type"] = "text/html";
            response->Header()->Headers()["Server"] = "galay server";
            response->Body() = html::Html404NotFound;
            ctrl.lock()->PushResponse(response->EncodePdu());
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
        ctrl.lock()->PushResponse(response->EncodePdu());
    }
    co_await group.Wait();
    ctrl.lock()->Done();
    co_return galay::common::CoroutineStatus::kCoroutineFinished;
}