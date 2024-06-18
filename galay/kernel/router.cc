#include "router.h"
#include "controller.h"


void 
galay::GY_HttpRouter::Get(const std::string &path, std::function<GY_TcpCoroutine<galay::CoroutineStatus>(std::weak_ptr<GY_HttpController>)> func)
{
    RegisterRouter("GET", path, func);
}

void 
galay::GY_HttpRouter::Post(const std::string &path, std::function<GY_TcpCoroutine<galay::CoroutineStatus>(std::weak_ptr<GY_HttpController>)> func)
{
    RegisterRouter("POST", path, func);
}

void 
galay::GY_HttpRouter::RegisterRouter(const std::string &mehtod, const std::string &path, std::function<GY_TcpCoroutine<galay::CoroutineStatus>(std::weak_ptr<GY_HttpController>)> func)
{
    this->m_routes[mehtod][path] = func;
}


galay::GY_TcpCoroutine<galay::CoroutineStatus>
galay::GY_HttpRouter::RouteHttp(std::weak_ptr<GY_Controller> ctrl)
{
    auto request = std::dynamic_pointer_cast<protocol::http::Http1_1_Request>(ctrl.lock()->GetRequest());
    WaitGroup group;
    galay::GY_HttpController::ptr http_ctrl = std::make_shared<galay::GY_HttpController>(ctrl);
    http_ctrl->SetWaitGroup(&group);
    auto method_routers = m_routes.find(request->GetMethod());
    if (method_routers != m_routes.end())
    {
        auto router = method_routers->second.find(request->GetUri());
        if (router != method_routers->second.end())
        {
            group.Add(1);
            http_ctrl->SetRequest(request);
            router->second(http_ctrl);
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
            ctrl.lock()->PushResponse(response);
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
        ctrl.lock()->PushResponse(response);
    }
    co_await group.Wait();
    ctrl.lock()->Done();
    co_return galay::CoroutineStatus::kCoroutineFinished;
}