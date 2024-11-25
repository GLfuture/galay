#include "Coroutine.h"
#include "Event.h"
#include "Awaiter.h"
#include "Scheduler.h"
#include "WaitAction.h"
#include "ExternApi.h"
#include <spdlog/spdlog.h>

namespace galay::coroutine
{
Coroutine Coroutine::promise_type::get_return_object() noexcept
{
    this->m_store = GetThisThreadCoroutineStore();
    this->m_coroutine = new Coroutine(std::coroutine_handle<promise_type>::from_promise(*this));
    this->m_store->AddCoroutine(m_coroutine);
    return *this->m_coroutine;
}

void Coroutine::promise_type::return_void() noexcept
{ 
    this->m_store->RemoveCoroutine(m_coroutine); 
}

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

void CoroutineStore::Clear()
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
    this->m_node = nullptr;
    this->m_awaiter = nullptr;
}

Coroutine::Coroutine(Coroutine&& other) noexcept
{
    this->m_handle = other.m_handle;
    other.m_handle = nullptr;
    this->m_node = other.m_node;
    other.m_node = nullptr;
    this->m_awaiter.store(other.m_awaiter);
    other.m_awaiter = nullptr;
}

Coroutine::Coroutine(const Coroutine& other) noexcept
{
    this->m_awaiter.store(other.m_awaiter);
    this->m_handle = other.m_handle;
    this->m_node = other.m_node;
}

Coroutine&
Coroutine::operator=(Coroutine&& other) noexcept
{
    this->m_handle = other.m_handle;
    other.m_handle = nullptr;
    this->m_node = other.m_node;
    other.m_node = nullptr;
    this->m_awaiter.store(other.m_awaiter);
    other.m_awaiter = nullptr;
    return *this;
}

void Coroutine::Destroy()
{
    m_handle.promise().GetStore()->RemoveCoroutine(this);
    m_handle.destroy();
}

bool Coroutine::SetAwaiter(Awaiter *awaiter)
{
    spdlog::info("SetAwaiter begin: {}", (void*)awaiter);
    Awaiter* t = m_awaiter.load();
    if(!m_awaiter.compare_exchange_strong(t, awaiter)) {
        return false;
    }
    spdlog::info("SetAwaiter after: {}", (void*)m_awaiter.load());
    return true;
}

Awaiter *Coroutine::GetAwaiter()
{
    spdlog::info("GetAwaiter: {}", (void*)m_awaiter.load());
    return m_awaiter.load();
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
    return Awaiter_bool(m_action, nullptr);
}

bool CoroutineWaiters::Decrease()
{
    if( m_num.load() == 0) return false;
    this->m_num.fetch_sub(1);
    if( ! m_action ) return true;
    if( m_num.load() == 0 )
    {
        m_action->GetCoroutine()->GetAwaiter()->SetResult(true);
        m_scheduler->EnqueueCoroutine(m_action->GetCoroutine());
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

namespace galay::this_coroutine
{
    
coroutine::Awaiter_void GetThisCoroutine(coroutine::Coroutine*& coroutine)
{
    action::GetCoroutineHandleAction* action = new action::GetCoroutineHandleAction(&coroutine);
    return coroutine::Awaiter_void(action, nullptr);
}

static thread_local action::TimeEventAction g_time_action;

coroutine::Awaiter_bool SleepFor(int64_t ms, std::shared_ptr<event::Timer>* timer, scheduler::CoroutineScheduler* scheduler)
{
    if(GetTimerSchedulerNum() == 0) {
        return coroutine::Awaiter_bool(false);
    }
    g_time_action.CreateTimer(ms, timer, [scheduler](event::Timer::ptr timer){
        auto co = std::any_cast<coroutine::Coroutine*>(timer->GetContext());
        co->GetAwaiter()->SetResult(true);
        if(scheduler == nullptr) {
            GetCoroutineSchedulerInOrder()->EnqueueCoroutine(co);
        } else{
            scheduler->EnqueueCoroutine(co);
        }
    });
    return coroutine::Awaiter_bool(&g_time_action , nullptr);
}
}