#include "controller.h"
#include "poller.h"

namespace galay::server
{

NetResult::NetResult(result::ResultInterface::ptr result)
{
    this->m_result = result;
}

bool 
NetResult::Success()
{
    return this->m_result->Success();
}

std::string 
NetResult::Error()
{
    return this->m_result->Error();
}

coroutine::GroupAwaiter& 
NetResult::Wait()
{
    return this->m_result->Wait();
}

Controller::Controller(std::weak_ptr<poller::TcpConnector> connector)
{
    this->m_connector = connector;
}

void 
Controller::SetContext(std::any&& context)
{
    this->m_context = std::forward<std::any>(context);
}

std::shared_ptr<poller::Timer> 
Controller::AddTimer(uint64_t during, uint32_t exec_times, std::function<void(std::shared_ptr<poller::Timer>)> &&func)
{
    return this->m_connector.lock()->AddTimer(during, exec_times, std::forward<std::function<void(std::shared_ptr<poller::Timer>)>>(func));
}

galay::protocol::Request::ptr
Controller::GetRequest()
{
    return this->m_connector.lock()->GetRequest();
}
void 
Controller::PopRequest()
{
    this->m_connector.lock()->PopRequest();
}

NetResult
Controller::Send(std::string&& response)
{
    auto ret = this->m_connector.lock()->Send(std::forward<std::string>(response));
    return NetResult(ret);
}

std::any&
Controller::GetContext()
{
    return this->m_context;
}

void 
Controller::Close()
{
    this->m_connector.lock()->Close();
}

HttpController::HttpController(Controller::ptr ctrl)
{
    this->m_ctrl = ctrl;
}

void 
HttpController::Close()
{
    this->m_ctrl->Close();
}

void 
HttpController::SetContext(std::any&& context)
{
    this->m_ctrl->SetContext(std::forward<std::any&&>(context));
}

std::any&
HttpController::GetContext()
{
    return this->m_ctrl->GetContext();
}

std::shared_ptr<poller::Timer> 
HttpController::AddTimer(uint64_t during, uint32_t exec_times, std::function<void(std::shared_ptr<poller::Timer>)> &&func)
{
    return this->m_ctrl->AddTimer(during,exec_times,std::forward<std::function<void(std::shared_ptr<poller::Timer>)>>(func));
}

protocol::http::HttpRequest::ptr 
HttpController::GetRequest()
{
    auto request = std::dynamic_pointer_cast<protocol::http::HttpRequest>(this->m_ctrl->GetRequest());
    this->m_ctrl->PopRequest();
    return request;
}

NetResult
HttpController::Send(std::string &&response)
{
    return this->m_ctrl->Send(std::forward<std::string>(response));
}

}
