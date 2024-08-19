#include "controller.h"
#include "scheduler.h"
#include "objector.h"
#include "result.h"

namespace galay{
namespace server{
GY_Controller::GY_Controller(std::weak_ptr<objector::GY_TcpConnector> connector)
{
    this->m_connector = connector;
}

void 
GY_Controller::SetContext(std::any&& context)
{
    this->m_context = std::forward<std::any>(context);
}

std::shared_ptr<objector::Timer> 
GY_Controller::AddTimer(uint64_t during, uint32_t exec_times, std::function<void(std::shared_ptr<objector::Timer>)> &&func)
{
    return this->m_connector.lock()->AddTimer(during, exec_times, std::forward<std::function<void(std::shared_ptr<objector::Timer>)>>(func));
}

void 
GY_Controller::SetError(std::any &&error)
{
    this->m_error = std::forward<std::any>(error);
}

galay::protocol::GY_Request::ptr
GY_Controller::GetRequest()
{
    return this->m_connector.lock()->GetRequest();
}

std::any&
GY_Controller::GetError()
{
    return m_error;
}

void 
GY_Controller::PopRequest()
{
    this->m_connector.lock()->PopRequest();
}

result::NetResult::ptr 
GY_Controller::Send(std::string&& response)
{
    return this->m_connector.lock()->Send(std::forward<std::string>(response));
}

std::any&
GY_Controller::GetContext()
{
    return this->m_context;
}

void 
GY_Controller::Close()
{
    this->m_connector.lock()->Close();
}

GY_HttpController::GY_HttpController(GY_Controller::ptr ctrl)
{
    this->m_ctrl = ctrl;
}

void 
GY_HttpController::Close()
{
    this->m_ctrl->Close();
}

galay::protocol::http::error::HttpError 
GY_HttpController::GetError()
{
    if(!this->m_ctrl->GetError().has_value()){
        return galay::protocol::http::error::HttpError{};
    }
    return std::any_cast<protocol::http::error::HttpError>(this->m_ctrl->GetError());
}

void 
GY_HttpController::SetContext(std::any&& context)
{
    this->m_ctrl->SetContext(std::forward<std::any&&>(context));
}

std::any&
GY_HttpController::GetContext()
{
    return this->m_ctrl->GetContext();
}

std::shared_ptr<objector::Timer> 
GY_HttpController::AddTimer(uint64_t during, uint32_t exec_times, std::function<void(std::shared_ptr<objector::Timer>)> &&func)
{
    return this->m_ctrl->AddTimer(during,exec_times,std::forward<std::function<void(std::shared_ptr<objector::Timer>)>>(func));
}

galay::protocol::http::HttpRequest::ptr 
GY_HttpController::GetRequest()
{
    auto request = std::dynamic_pointer_cast<protocol::http::HttpRequest>(this->m_ctrl->GetRequest());
    this->m_ctrl->PopRequest();
    return request;
}

galay::result::NetResult::ptr
GY_HttpController::Send(std::string &&response)
{
    return this->m_ctrl->Send(std::forward<std::string>(response));
}

}}
