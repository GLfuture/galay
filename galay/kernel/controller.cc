#include "controller.h"
#include "scheduler.h"
#include "objector.h"

galay::GY_Controller::GY_Controller(::std::weak_ptr<GY_Connector> connector)
{
    this->m_connector = connector;
    this->m_close = false;
}

void 
galay::GY_Controller::SetContext(::std::any&& context)
{
    this->m_connector.lock()->SetContext(::std::forward<::std::any&&>(context));
}

// void 
// galay::GY_Controller::SetThisCoroutine(GY_TcpCoroutine<CoroutineStatus>&& coroutine)
// {
//     this->m_coroutine = std::forward<GY_TcpCoroutine<CoroutineStatus>&&>(coroutine);
// }

::std::shared_ptr<galay::Timer> 
galay::GY_Controller::AddTimer(uint64_t during, uint32_t exec_times,::std::function<::std::any()> &&func)
{
    return this->m_connector.lock()->AddTimer(during, exec_times, std::forward<::std::function<::std::any()>&&>(func));
}

galay::protocol::GY_TcpRequest::ptr
galay::GY_Controller::GetRequest()
{
    return this->m_connector.lock()->GetRequest();
}

void 
galay::GY_Controller::PushResponse(galay::protocol::GY_TcpResponse::ptr response)
{
    this->m_connector.lock()->PushResponse(response);
}

::std::any&&
galay::GY_Controller::GetContext()
{
    return this->m_connector.lock()->GetContext();
}

// galay::GY_TcpCoroutine<galay::CoroutineStatus>& 
// galay::GY_Controller::GetThisCoroutine()
// {
//     if(&(this->m_coroutine)) return this->m_coroutine;
//     throw std::runtime_error("m_coroutine is not valid");
// }

void 
galay::GY_Controller::Close()
{
    this->m_close = true;
}

bool 
galay::GY_Controller::IsClosed()
{
    return this->m_close;
}

void 
galay::GY_Controller::Done()
{
    m_group->Done();
}

void 
galay::GY_Controller::SetWaitGroup(WaitGroup* waitgroup)
{
    this->m_group = waitgroup;
}
