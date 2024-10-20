#include "Coroutine.h"
#include "Awaiter.h"
#include "Scheduler.h"
#include "WaitAction.h"

namespace galay::coroutine
{

void CoroutineStore::AddCoroutine(Coroutine *co)
{
    auto node = m_coroutines.PushBack(co);
    co->GetListNode() = node;
}

void CoroutineStore::RemoveCoroutine(Coroutine *co)
{
    m_coroutines.Remove(co->GetListNode());
    co->GetListNode() = nullptr;
}

CoroutineStore g_coroutine_store;

CoroutineStore::~CoroutineStore()
{
    while(!m_coroutines.Empty())
    {
        auto node = m_coroutines.PopFront();
        if(node){
            node->m_data->Destroy();
            delete node;
        }
    }
}

Coroutine::Coroutine(std::coroutine_handle<promise_type> handle) noexcept
{
    this->m_handle = handle;
    this->m_context = std::make_shared<std::any>();
    this->m_node = nullptr;
}

Coroutine::Coroutine(Coroutine&& other) noexcept
{
    this->m_context.swap(other.m_context);
    other.m_context.reset();
    this->m_handle = other.m_handle;
    other.m_handle = nullptr;
    this->m_node = other.m_node;
    other.m_node = nullptr;
}

Coroutine::Coroutine(const Coroutine& other) noexcept
{
    this->m_context = other.m_context;
    this->m_handle = other.m_handle;
    this->m_node = other.m_node;
}

Coroutine&
Coroutine::operator=(Coroutine&& other) noexcept
{
    this->m_context.swap(other.m_context);
    other.m_context.reset();
    this->m_handle = other.m_handle;
    other.m_handle = nullptr;
    this->m_node = other.m_node;
    other.m_node = nullptr;
    return *this;
}

void Coroutine::Destroy()
{
    g_coroutine_store.RemoveCoroutine(m_handle.promise().GetCoroutine());
    m_handle.destroy();
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
        m_scheduler->ResumeCoroutine(m_action->GetCoroutine());
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

Awaiter_void GetThisCoroutine(Coroutine*& coroutine)
{
    action::GetCoroutineHandleAction* action = new action::GetCoroutineHandleAction(&coroutine);
    return Awaiter_void(action);
}



}