#include "Coroutine.h"
#include "Event.h"
#include "Awaiter.h"
#include "Scheduler.h"
#include "WaitAction.h"
#include "ExternApi.h"
#include <spdlog/spdlog.h>

namespace galay::coroutine
{
CoroutineStore::CoroutineStore()
{
    m_coroutines.reserve(DEFAULT_COROUTINE_STORE_HASH_SIZE);
}

void CoroutineStore::Reserve(size_t size)
{
    m_coroutines.reserve(size);
}

void CoroutineStore::AddCoroutine(Coroutine *co)
{
    std::lock_guard lk(m_mutex);
    auto it = m_coroutines.insert(co);
    if (!it.second) {
        spdlog::error("CoroutineStore::AddCoroutine Coroutine {}, Coroutine Add Failed", static_cast<void*>(co));
    }
}

void CoroutineStore::RemoveCoroutine(Coroutine *co)
{
    std::lock_guard lk(m_mutex);
    auto it = m_coroutines.find(co);
    if(it != m_coroutines.end()) m_coroutines.erase(it);
    else spdlog::error("CoroutineStore::RemoveCoroutine Coroutine {}, Coroutine Not Exist", static_cast<void*>(co));
}

void CoroutineStore::Clear()
{
    std::unique_lock lk(m_mutex);
    while(!m_coroutines.empty()) {
        auto it = m_coroutines.begin();
        lk.unlock();
        if(it != m_coroutines.end()) (*it)->Destroy();
        lk.lock();
    }
}

Coroutine Coroutine::promise_type::get_return_object() noexcept
{
    this->m_store = Global_GetCoroutineStore();
    this->m_coroutine = new Coroutine(std::coroutine_handle<promise_type>::from_promise(*this));
    this->m_store->AddCoroutine(m_coroutine);
    return *this->m_coroutine;
}

Coroutine::promise_type::~promise_type()
{
    this->m_store->RemoveCoroutine(m_coroutine);
    delete m_coroutine;
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
    this->m_awaiter.store(other.m_awaiter);
    other.m_awaiter = nullptr;
}

Coroutine::Coroutine(const Coroutine& other) noexcept
{
    this->m_awaiter.store(other.m_awaiter);
    this->m_handle = other.m_handle;
}

Coroutine&
Coroutine::operator=(Coroutine&& other) noexcept
{
    this->m_handle = other.m_handle;
    other.m_handle = nullptr;
    this->m_awaiter.store(other.m_awaiter);
    other.m_awaiter = nullptr;
    return *this;
}

void Coroutine::Destroy()
{
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