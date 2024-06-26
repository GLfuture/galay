#include "controller.h"
#include "scheduler.h"
#include "objector.h"

galay::kernel::GY_Controller::GY_Controller(std::weak_ptr<GY_Connector> connector)
{
    this->m_connector = connector;
    this->m_close = false;
}

void 
galay::kernel::GY_Controller::SetContext(std::any&& context)
{
    this->m_connector.lock()->SetContext(std::forward<std::any>(context));
}

// void 
// galay::kernel::GY_Controller::SetThisCoroutine(GY_TcpCoroutine<CoroutineStatus>&& coroutine)
// {
//     this->m_coroutine = std::forward<GY_TcpCoroutine<CoroutineStatus>&&>(coroutine);
// }

std::shared_ptr<galay::kernel::Timer> 
galay::kernel::GY_Controller::AddTimer(uint64_t during, uint32_t exec_times,std::function<std::any()> &&func)
{
    return this->m_connector.lock()->AddTimer(during, exec_times, std::forward<std::function<std::any()>&&>(func));
}

galay::protocol::GY_Request::ptr
galay::kernel::GY_Controller::GetRequest()
{
    return this->m_connector.lock()->GetRequest();
}

void 
galay::kernel::GY_Controller::PushResponse(std::string&& response)
{
    this->m_connector.lock()->PushResponse(std::forward<std::string>(response));
}

std::any&&
galay::kernel::GY_Controller::GetContext()
{
    return this->m_connector.lock()->GetContext();
}

// galay::GY_TcpCoroutine<galay::CoroutineStatus>& 
// galay::kernel::GY_Controller::GetThisCoroutine()
// {
//     if(&(this->m_coroutine)) return this->m_coroutine;
//     throw std::runtime_error("m_coroutine is not valid");
// }

void 
galay::kernel::GY_Controller::Close()
{
    this->m_close = true;
}

bool 
galay::kernel::GY_Controller::IsClosed()
{
    return this->m_close;
}

void 
galay::kernel::GY_Controller::Done()
{
    m_group->Done();
}

void 
galay::kernel::GY_Controller::SetWaitGroup(WaitGroup* waitgroup)
{
    this->m_group = waitgroup;
}


galay::kernel::GY_HttpController::GY_HttpController(GY_Controller::wptr ctrl)
{
    this->m_ctrl = ctrl;
}

void 
galay::kernel::GY_HttpController::Done()
{
    this->m_waitgroup->Done();
}

void 
galay::kernel::GY_HttpController::Close()
{
    this->m_ctrl.lock()->Close();
}

void 
galay::kernel::GY_HttpController::SetContext(std::any&& context)
{
    this->m_ctrl.lock()->SetContext(std::forward<std::any&&>(context));
}

std::any&& 
galay::kernel::GY_HttpController::GetContext()
{
    return this->m_ctrl.lock()->GetContext();
}

std::shared_ptr<galay::kernel::Timer> 
galay::kernel::GY_HttpController::AddTimer(uint64_t during, uint32_t exec_times,std::function<std::any()> &&func)
{
    return this->m_ctrl.lock()->AddTimer(during,exec_times,std::forward<std::function<std::any()>&&>(func));
}

void 
galay::kernel::GY_HttpController::SetWaitGroup(WaitGroup* waitgroup)
{
    this->m_waitgroup = waitgroup;
}

galay::protocol::http::Http1_1_Request::ptr 
galay::kernel::GY_HttpController::GetRequest()
{
    return this->m_request;
}


void 
galay::kernel::GY_HttpController::PushResponse(std::string&& response)
{
    this->m_ctrl.lock()->PushResponse(std::forward<std::string>(response));
}

void 
galay::kernel::GY_HttpController::SetRequest(protocol::http::Http1_1_Request::ptr request)
{
    this->m_request = request;
}
