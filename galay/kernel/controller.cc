#include "controller.h"
#include "scheduler.h"
#include "objector.h"
#include "result.h"

galay::kernel::GY_Controller::GY_Controller(std::weak_ptr<GY_TcpConnector> connector)
{
    this->m_connector = connector;
}

void 
galay::kernel::GY_Controller::SetContext(std::any&& context)
{
    this->m_connector.lock()->SetContext(std::forward<std::any>(context));
}

std::shared_ptr<galay::kernel::Timer> 
galay::kernel::GY_Controller::AddTimer(uint64_t during, uint32_t exec_times, std::function<void(std::shared_ptr<Timer>)> &&func)
{
    return this->m_connector.lock()->AddTimer(during, exec_times, std::forward<std::function<void(std::shared_ptr<Timer>)>>(func));
}

galay::protocol::GY_Request::ptr
galay::kernel::GY_Controller::GetRequest()
{
    return this->m_connector.lock()->GetRequest();
}

void 
galay::kernel::GY_Controller::PopRequest()
{
    this->m_connector.lock()->PopRequest();
}

galay::kernel::NetResult::ptr 
galay::kernel::GY_Controller::Send(std::string&& response)
{
    return this->m_connector.lock()->Send(std::forward<std::string>(response));
}

std::any&
galay::kernel::GY_Controller::GetContext()
{
    return this->m_connector.lock()->GetContext();
}

void 
galay::kernel::GY_Controller::Close()
{
    this->m_connector.lock()->Close();
}

galay::kernel::GY_HttpController::GY_HttpController(GY_Controller::ptr ctrl)
{
    this->m_ctrl = ctrl;
}

void 
galay::kernel::GY_HttpController::Close()
{
    this->m_ctrl->Close();
}

void 
galay::kernel::GY_HttpController::SetContext(std::any&& context)
{
    this->m_ctrl->SetContext(std::forward<std::any&&>(context));
}

std::any&
galay::kernel::GY_HttpController::GetContext()
{
    return this->m_ctrl->GetContext();
}

std::shared_ptr<galay::kernel::Timer> 
galay::kernel::GY_HttpController::AddTimer(uint64_t during, uint32_t exec_times, std::function<void(std::shared_ptr<Timer>)> &&func)
{
    return this->m_ctrl->AddTimer(during,exec_times,std::forward<std::function<void(std::shared_ptr<Timer>)>>(func));
}

galay::protocol::http::HttpRequest::ptr 
galay::kernel::GY_HttpController::GetRequest()
{
    auto request = std::dynamic_pointer_cast<protocol::http::HttpRequest>(this->m_ctrl->GetRequest());
    this->m_ctrl->PopRequest();
    return request;
}

galay::kernel::NetResult::ptr
galay::kernel::GY_HttpController::Send(std::string &&response)
{
    return this->m_ctrl->Send(std::forward<std::string>(response));
}