#include "builder.h"

void galay::GY_HttpServerBuilder::Get(const std::string &path, std::function<GY_TcpCoroutine<galay::CoroutineStatus>(GY_HttpController::wptr)> func)
{
    RegisterRouter("GET", path, func);
}

void galay::GY_HttpServerBuilder::Post(const std::string &path, std::function<GY_TcpCoroutine<galay::CoroutineStatus>(GY_HttpController::wptr)> func)
{
    RegisterRouter("POST", path, func);
}

void galay::GY_HttpServerBuilder::RegisterRouter(const std::string &mehtod, const std::string &path, std::function<GY_TcpCoroutine<galay::CoroutineStatus>(GY_HttpController::wptr)> func)
{
    this->m_routers[mehtod][path] = func;
}

::std::function<galay::GY_TcpCoroutine<galay::CoroutineStatus>(galay::GY_Controller::wptr)>
galay::GY_HttpServerBuilder::GetUserFunction()
{
    return [this](galay::GY_Controller::wptr ctrl){
        return RouteHttp(ctrl);
    };
}

galay::GY_TcpCoroutine<galay::CoroutineStatus>
galay::GY_HttpServerBuilder::RouteHttp(galay::GY_Controller::wptr ctrl)
{
    auto request = std::dynamic_pointer_cast<protocol::http::Http1_1_Request>(ctrl.lock()->GetRequest());
    WaitGroup group;
    galay::GY_HttpController::ptr http_ctrl = std::make_shared<galay::GY_HttpController>(ctrl);
    http_ctrl->SetWaitGroup(&group);
    auto method_routers = m_routers.find(request->GetMethod());
    if (method_routers != m_routers.end())
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

void galay::GY_HttpServerBuilder::SetUserFunction(::std::pair<uint16_t, ::std::function<GY_TcpCoroutine<galay::CoroutineStatus>(GY_Controller::wptr)>> port_func)
{
    GY_TcpServerBuilder<protocol::http::Http1_1_Request, protocol::http::Http1_1_Response>::SetUserFunction(port_func);
}
