#include "Coroutine.h"
#include "Event.h"
#include "Awaiter.h"
#include "Scheduler.h"
#include "WaitAction.h"
#include "ExternApi.h"

namespace galay::coroutine
{
Coroutine Coroutine::promise_type::get_return_object() noexcept
{
    this->m_store = Global_GetCoroutineStore();
    this->m_coroutine = new Coroutine(std::coroutine_handle<promise_type>::from_promise(*this));
    this->m_store->AddCoroutine(m_coroutine);
    return *this->m_coroutine;
}

void Coroutine::promise_type::return_void() const noexcept
{ 
    this->m_store->RemoveCoroutine(m_coroutine); 
}

void CoroutineStore::AddCoroutine(Coroutine *co)
{
    std::lock_guard lk(m_mutex);
    m_coroutines.push_back(co);
    co->GetListNode() = std::prev(m_coroutines.end());
}

void CoroutineStore::RemoveCoroutine(Coroutine *co)
{
    if(!co->GetListNode().has_value()) return;
    std::lock_guard lk(m_mutex);
    m_coroutines.erase(co->GetListNode().value());
    co->GetListNode().reset();
}

void CoroutineStore::Clear()
{
    std::lock_guard lk(m_mutex);
    while(!m_coroutines.empty())
    {
        auto node = m_coroutines.front();
        m_coroutines.pop_front();
        if(node){
            node->Destroy();
        }
    }
}

Coroutine::Coroutine(const std::coroutine_handle<promise_type> handle) noexcept
{
    this->m_handle = handle;
    this->m_awaiter = nullptr;
}

Coroutine::Coroutine(Coroutine&& other) noexcept
{
    this->m_handle = other.m_handle;
    other.m_handle = nullptr;
    this->m_node = other.m_node;
    other.m_node.reset();
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
    other.m_node.reset();
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
    if(Awaiter* t = m_awaiter.load(); !m_awaiter.compare_exchange_strong(t, awaiter)) {
        return false;
    }
    return true;
}

Awaiter *Coroutine::GetAwaiter() const
{
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
    return {m_action, nullptr};
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

void CoroutineWaitContext::AddWaitCoroutineNum(const int num) const
{
    m_waiters.AddCoroutineTaskNum(num);
}

bool CoroutineWaitContext::Done() const
{
    return m_waiters.Decrease();
}

}

namespace galay::this_coroutine
{
    
coroutine::Awaiter_void GetThisCoroutine(coroutine::Coroutine*& coroutine)
{
    auto* action = new action::GetCoroutineHandleAction(&coroutine);
    return {action, nullptr};
}

static thread_local action::TimeEventAction g_time_action;

coroutine::Awaiter_bool SleepFor(int64_t ms, std::shared_ptr<event::Timer>* timer, scheduler::CoroutineScheduler* scheduler)
{
    if(GetTimerSchedulerNum() == 0) {
        return coroutine::Awaiter_bool(false);
    }
    g_time_action.CreateTimer(ms, timer, [scheduler](const event::Timer::ptr& use_timer){
        const auto co = std::any_cast<coroutine::Coroutine*>(use_timer->GetContext());
        co->GetAwaiter()->SetResult(true);
        if(scheduler == nullptr) {
            GetCoroutineSchedulerInOrder()->EnqueueCoroutine(co);
        } else{
            scheduler->EnqueueCoroutine(co);
        }
    });
    return {&g_time_action , nullptr};
}
}