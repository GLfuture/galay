#include "builder.h"
#include "router.h"

std::function<galay::kernel::GY_TcpCoroutine<galay::kernel::CoroutineStatus>(galay::kernel::GY_Controller::wptr)>
galay::kernel::GY_HttpServerBuilder::GetUserFunction()
{
    return [this](galay::kernel::GY_Controller::wptr ctrl){
        return this->m_router->RouteHttp(ctrl);
    };
}

void 
galay::kernel::GY_HttpServerBuilder::SetRouter(std::shared_ptr<GY_HttpRouter> router)
{
    this->m_router = router;
}


void galay::kernel::GY_HttpServerBuilder::SetUserFunction(std::pair<uint16_t, std::function<GY_TcpCoroutine<galay::kernel::CoroutineStatus>(GY_Controller::wptr)>> port_func)
{
    GY_TcpServerBuilder<protocol::http::Http1_1_Request, protocol::http::Http1_1_Response>::SetUserFunction(port_func);
}
