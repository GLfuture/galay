#include "router.h"
#include "controller.h"


void 
galay::kernel::GY_HttpRouter::Get(const std::string &path, std::function<common::GY_TcpCoroutine<common::CoroutineStatus>(std::weak_ptr<GY_HttpController>)> func)
{
    RegisterRouter("GET", path, func);
}

void 
galay::kernel::GY_HttpRouter::Post(const std::string &path, std::function<common::GY_TcpCoroutine<common::CoroutineStatus>(std::weak_ptr<GY_HttpController>)> func)
{
    RegisterRouter("POST", path, func);
}

void 
galay::kernel::GY_HttpRouter::Options(const std::string &path, std::function<common::GY_TcpCoroutine<common::CoroutineStatus>(std::weak_ptr<GY_HttpController>)> func)
{
    RegisterRouter("OPTIONS", path, func);
}

void 
galay::kernel::GY_HttpRouter::Put(const std::string &path, std::function<common::GY_TcpCoroutine<common::CoroutineStatus>(std::weak_ptr<GY_HttpController>)> func)
{
    RegisterRouter("PUT", path, func);
}

void 
galay::kernel::GY_HttpRouter::Delete(const std::string &path, std::function<common::GY_TcpCoroutine<common::CoroutineStatus>(std::weak_ptr<GY_HttpController>)> func)
{
    RegisterRouter("DELETE", path, func);
}

void 
galay::kernel::GY_HttpRouter::Patch(const std::string &path, std::function<common::GY_TcpCoroutine<common::CoroutineStatus>(std::weak_ptr<GY_HttpController>)> func)
{
    RegisterRouter("PATCH", path, func);
}

void 
galay::kernel::GY_HttpRouter::Head(const std::string &path, std::function<common::GY_TcpCoroutine<common::CoroutineStatus>(std::weak_ptr<GY_HttpController>)> func)
{
    RegisterRouter("HEAD", path, func);
}

void 
galay::kernel::GY_HttpRouter::Trace(const std::string &path, std::function<common::GY_TcpCoroutine<common::CoroutineStatus>(std::weak_ptr<GY_HttpController>)> func)
{
    RegisterRouter("TRACE", path, func);
}

void 
galay::kernel::GY_HttpRouter::Connect(const std::string &path, std::function<common::GY_TcpCoroutine<common::CoroutineStatus>(std::weak_ptr<GY_HttpController>)> func)
{
    RegisterRouter("CONNECT", path, func);
}


void 
galay::kernel::GY_HttpRouter::RegisterRouter(const std::string &mehtod, const std::string &path, std::function<common::GY_TcpCoroutine<common::CoroutineStatus>(std::weak_ptr<GY_HttpController>)> func)
{
    this->m_routes[mehtod][path] = func;
}


galay::common::GY_TcpCoroutine<galay::common::CoroutineStatus>
galay::kernel::GY_HttpRouter::RouteHttp(std::weak_ptr<GY_Controller> ctrl)
{
    auto request = std::dynamic_pointer_cast<protocol::http::Http1_1_Request>(ctrl.lock()->GetRequest());
    WaitGroup group;
    galay::kernel::GY_HttpController::ptr http_ctrl = std::make_shared<galay::kernel::GY_HttpController>(ctrl);
    http_ctrl->SetWaitGroup(&group);
    auto method_routers = m_routes.find(request->GetMethod());
    if (method_routers != m_routes.end())
    {
        auto router = method_routers->second.find(request->GetUri());
        if (router != method_routers->second.end())
        {
            group.Add(1);
            http_ctrl->SetRequest(request);
            m_coroBusiness = router->second(http_ctrl);
        }
        else
        {
            auto response = std::make_shared<protocol::http::Http1_1_Response>();
            response->SetStatus(protocol::http::Http1_1_Response::NotFound_404);
            response->SetVersion("1.1");
            response->SetHeadPair({"Connection", "close"});
            response->SetHeadPair({"Content-Type", "text/html"});
            response->SetHeadPair({"Server", "galay server"});
            response->SetBody("<html><head><meta charset=\"utf-8\"><title>404</title></head><body>Not Found</body></html>");
            ctrl.lock()->PushResponse(response->EncodePdu());
        }
    }
    else
    {
        auto response = std::make_shared<protocol::http::Http1_1_Response>();
        response->SetStatus(protocol::http::Http1_1_Response::MethodNotAllowed_405);
        response->SetVersion("1.1");
        response->SetHeadPair({"Connection", "close"});
        response->SetHeadPair({"Content-Type", "text/html"});
        response->SetHeadPair({"Server", "galay server"});
        response->SetBody("<html><head><meta charset=\"utf-8\"><title>405</title></head><body>Method Not Allowed</body></html>");
        ctrl.lock()->PushResponse(response->EncodePdu());
    }
    co_await group.Wait();
    ctrl.lock()->Done();
    co_return galay::common::CoroutineStatus::kCoroutineFinished;
}