#include "Coroutine.h"
#include "Event.h"
#include "Awaiter.h"
#include "Scheduler.h"
#include "WaitAction.h"
#include "ExternApi.h"
#include "Log.h"

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
    std::unique_lock lk(m_mutex);
    auto it = m_coroutines.insert(co);
    if (!it.second) {
        LogError("[Add coroutine failed, coroutine: {}]", static_cast<void*>(co));
    }
}

void CoroutineStore::RemoveCoroutine(Coroutine *co)
{
    std::unique_lock lk(m_mutex);
    auto it = m_coroutines.find(co);
    if(it != m_coroutines.end()) {
        m_coroutines.erase(it);
        delete co;
    }
    else LogError("[Remove coroutine failed, coroutine: {}]", static_cast<void*>(co));
}

bool CoroutineStore::Exist(Coroutine *co)
{
    std::shared_lock lk(m_mutex);
    return m_coroutines.contains(co);
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
    this->m_store = GetCoroutineStore();
    this->m_coroutine = new Coroutine(std::coroutine_handle<promise_type>::from_promise(*this));
    this->m_store->AddCoroutine(m_coroutine);
    return *this->m_coroutine;
}

Coroutine::promise_type::~promise_type()
{
    m_coroutine->Exit();
    this->m_store->RemoveCoroutine(m_coroutine);
}

Coroutine::Coroutine(const std::coroutine_handle<promise_type> handle) noexcept
{
    this->m_handle = handle;
    this->m_awaiter = nullptr;
    this->m_exit_cb = nullptr;
}

Coroutine::Coroutine(Coroutine&& other) noexcept
{
    this->m_handle = other.m_handle;
    other.m_handle = nullptr;
    this->m_awaiter.store(other.m_awaiter);
    other.m_awaiter = nullptr;
    this->m_exit_cb = other.m_exit_cb;
    other.m_exit_cb = nullptr;
}

Coroutine::Coroutine(const Coroutine& other) noexcept
{
    this->m_awaiter.store(other.m_awaiter);
    this->m_handle = other.m_handle;
    this->m_exit_cb = other.m_exit_cb;
}

Coroutine&
Coroutine::operator=(Coroutine&& other) noexcept
{
    this->m_handle = other.m_handle;
    other.m_handle = nullptr;
    this->m_awaiter.store(other.m_awaiter);
    other.m_awaiter = nullptr;
    this->m_exit_cb = std::move(other.m_exit_cb);
    other.m_exit_cb = nullptr;
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

void Coroutine::SetExitCallback(const std::function<void()>& callback)
{
    this->m_exit_cb = callback;
}

Awaiter *Coroutine::GetAwaiter() const
{
    return m_awaiter.load();
}

void Coroutine::Exit()
{
    if(m_exit_cb) m_exit_cb();
}

}

namespace galay::this_coroutine
{
    
coroutine::Awaiter_void GetThisCoroutine(coroutine::Coroutine*& coroutine)
{
    auto* action = new action::CoroutineHandleAction([&coroutine](coroutine::Coroutine* co, void* ctx)->bool{
        coroutine = co;
        return false;
    });
    return {action, nullptr};
}

static thread_local action::TimeEventAction GlobalTimeAction;

coroutine::Awaiter_bool SleepFor(int64_t ms, std::shared_ptr<event::Timer>* timer, scheduler::CoroutineScheduler* scheduler)
{
    if(GetTimerSchedulerNum() == 0) {
        return coroutine::Awaiter_bool(false);
    }
    GlobalTimeAction.CreateTimer(ms, timer, [scheduler](const event::Timer::ptr& use_timer){
        const auto co = std::any_cast<coroutine::Coroutine*>(use_timer->GetContext());
        co->GetAwaiter()->SetResult(true);
        if(scheduler == nullptr) {
            GetCoroutineSchedulerInOrder()->EnqueueCoroutine(co);
        } else{
            scheduler->EnqueueCoroutine(co);
        }
    });
    return {&GlobalTimeAction , nullptr};
}


coroutine::Awaiter_void Exit(const std::function<void(void)>& callback)
{
    auto* action = new action::CoroutineHandleAction([callback](coroutine::Coroutine* co, void* ctx)->bool{
        co->SetExitCallback(std::move(callback));
        return false;
    });
    return coroutine::Awaiter_void();
}
}