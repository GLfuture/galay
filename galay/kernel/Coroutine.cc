#include "Coroutine.h"
#include "Awaiter.h"
#include "Scheduler.h"
#include "WaitAction.h"

namespace galay::coroutine
{
#include "Coroutine.h"

Coroutine::Coroutine(std::coroutine_handle<promise_type> handle) noexcept
{
    this->m_handle = handle;
    this->m_context = std::make_shared<std::any>();
}

Coroutine::Coroutine(Coroutine&& other) noexcept
{
    this->m_context.swap(other.m_context);
    other.m_context.reset();
    this->m_handle = other.m_handle;
    other.m_handle = nullptr;
}

Coroutine::Coroutine(const Coroutine& other) noexcept
{
    this->m_context = other.m_context;
    this->m_handle = other.m_handle;
}

Coroutine&
Coroutine::operator=(Coroutine&& other) noexcept
{
    this->m_context.swap(other.m_context);
    other.m_context.reset();
    this->m_handle = other.m_handle;
    other.m_handle = nullptr;
    return *this;
}

CoroutineWaiters::CoroutineWaiters(int num, scheduler::CoroutineScheduler* scheduler)
{
    this->m_num.store(num);
    this->m_action = nullptr;
    this->m_scheduler = scheduler;
}

Awaiter_bool CoroutineWaiters::Wait(int timeout)
{
    if( m_num.load() == 0) {
        return Awaiter_bool(true);
    }
    if( !m_action ) m_action = new action::CoroutineWaitAction();
    return Awaiter_bool(m_action);
}

bool CoroutineWaiters::Decrease()
{
    if( m_num.load() == 0) return false;
    this->m_num.fetch_sub(1);
    if( ! m_action ) return true;
    if( m_num.load() == 0 )
    {
        m_action->GetCoroutine()->SetContext(true);
        m_scheduler->AddCoroutine(m_action->GetCoroutine());
    }
    return true;
}

CoroutineWaiters::~CoroutineWaiters()
{
    if( m_action ) delete m_action;
}

CoroutineWaitContext::CoroutineWaitContext(CoroutineWaiters& waiters)
    :m_waiters(waiters)
{
}

void CoroutineWaitContext::AddWaitCoroutineNum(int num)
{
    m_waiters.AddCoroutineTaskNum(num);
}

bool CoroutineWaitContext::Done()
{
    return m_waiters.Decrease();
}

}