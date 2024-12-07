#include "Coroutine.h"
#include "Event.h"
#include "Awaiter.h"
#include "Scheduler.h"
#include "WaitAction.h"
#include "ExternApi.h"
#include "Log.h"
#include <iostream>

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
    m_coroutine = new Coroutine(std::coroutine_handle<promise_type>::from_promise(*this), CoroutineSchedulerHolder::GetInstance()->GetScheduler());
    return *m_coroutine;
}

Coroutine::promise_type::~promise_type()
{
    //防止协程运行时resume创建出新的协程，析构时重复析构
    if(m_coroutine) {
        m_coroutine->Exit();
        delete m_coroutine;
    }
}

Coroutine::Coroutine(const std::coroutine_handle<promise_type> handle, details::CoroutineScheduler* scheduler) noexcept
{
    m_handle = handle;
    m_awaiter = nullptr;
    m_scheduler = scheduler;
}

Coroutine::Coroutine(Coroutine&& other) noexcept
{
    m_handle = other.m_handle;
    other.m_handle = nullptr;
    m_awaiter.store(other.m_awaiter);
    other.m_awaiter = nullptr;
    m_exit_cbs.splice(m_exit_cbs.begin(), other.m_exit_cbs);
}

Coroutine::Coroutine(const Coroutine& other) noexcept
{
    m_awaiter.store(other.m_awaiter);
    m_handle = other.m_handle;
    m_exit_cbs = other.m_exit_cbs;
}

Coroutine&
Coroutine::operator=(Coroutine&& other) noexcept
{
    m_handle = other.m_handle;
    other.m_handle = nullptr;
    m_awaiter.store(other.m_awaiter);
    other.m_awaiter = nullptr;
    m_exit_cbs.splice(m_exit_cbs.begin(), other.m_exit_cbs);
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

void Coroutine::AppendExitCallback(const std::function<void()>& callback)
{
    m_exit_cbs.emplace_front(callback);
}

Awaiter *Coroutine::GetAwaiter() const
{
    return m_awaiter.load();
}

void Coroutine::Exit()
{
    while(!m_exit_cbs.empty()) {
        m_exit_cbs.front()();
        m_exit_cbs.pop_front();
    }
}

WaitGroup::WaitGroup(uint32_t count)
    :m_count(count)
{
}

void WaitGroup::Done()
{
    std::shared_lock lk(m_mutex);
    if(m_count == 0) return;
    --m_count;
    if(m_count == 0) {
        if(m_coroutine) {
            m_coroutine->GetAwaiter()->SetResult(true);
            m_coroutine->BelongScheduler()->EnqueueCoroutine(m_coroutine);
        }
    }
}

void WaitGroup::Reset(uint32_t count)
{
    std::unique_lock lk(m_mutex);
    m_count = count;
    m_coroutine = nullptr;
}

Awaiter_bool WaitGroup::Wait()
{
    auto* action = new details::CoroutineHandleAction([this](coroutine::Coroutine* co, void* ctx)->bool{
        std::unique_lock lk(m_mutex);
        if(m_count == 0) {
            co->GetAwaiter()->SetResult(false);
            return false;
        }
        m_coroutine = co;
        return true;
    });
    return {action, nullptr};
}

Awaiter_void RoutineContext::DeferDone()
{
    auto* action = new details::CoroutineHandleAction([this](coroutine::Coroutine* co, void* ctx)->bool{
        m_coroutine = co;
        co->AppendExitCallback([this](){
            m_coroutine = nullptr;
        });
        return false;
    });
    return {action, nullptr};
}

void RoutineContext::BeCanceled()
{
    if(m_coroutine) {
        m_coroutine->Destroy();
    }
}

void RoutineContextCancel::operator()()
{
    if(!m_context.expired()) m_context.lock()->BeCanceled();
}

Awaiter_bool RoutineContextWithWaitGroup::Wait()
{
    return m_wait_group.Wait();
}

Awaiter_void RoutineContextWithWaitGroup::DeferDone()
{
    auto* action = new details::CoroutineHandleAction([this](coroutine::Coroutine* co, void* ctx)->bool{
        m_coroutine = co;
        co->AppendExitCallback([this](){
            m_wait_group.Done();
        });
        co->AppendExitCallback([this](){
            m_coroutine = nullptr;
        });
        return false;
    });
    return {action, nullptr};
}

void RoutineContextWithWaitGroup::BeCanceled()
{
    m_wait_group.Reset(0);
    if(m_coroutine) {
        m_coroutine->Destroy();
    }
}
}

namespace galay::coroutine
{
coroutine::CoroutineStore g_coroutine_store;

coroutine::CoroutineStore* GetCoroutineStore()
{
    return &g_coroutine_store;
}

std::pair<RoutineContext::ptr, RoutineContextCancel::ptr> ContextFactory::WithNewContext()
{
    RoutineContext::ptr context = std::make_shared<RoutineContext>();
    RoutineContextCancel::ptr cancel = std::make_shared<RoutineContextCancel>(context);
    return {context, cancel};
}

std::pair<RoutineContextWithWaitGroup::ptr, RoutineContextCancel::ptr> ContextFactory::WithWaitGroupContext()
{
    RoutineContextWithWaitGroup::ptr context = std::make_shared<RoutineContextWithWaitGroup>(1);
    RoutineContextCancel::ptr cancel = std::make_shared<RoutineContextCancel>(context);
    return {context, cancel};
}
}


namespace galay::this_coroutine
{
coroutine::Awaiter_void AddToCoroutineStore()
{
    auto* action = new details::CoroutineHandleAction([](coroutine::Coroutine* co, void* ctx)->bool{
        co->AppendExitCallback([co](){
            coroutine::GetCoroutineStore()->RemoveCoroutine(co);
        });
        coroutine::GetCoroutineStore()->AddCoroutine(co);
        return false;
    });
    return {action, nullptr};
}

coroutine::Awaiter_void GetThisCoroutine(coroutine::Coroutine*& coroutine)
{
    auto* action = new details::CoroutineHandleAction([&coroutine](coroutine::Coroutine* co, void* ctx)->bool{
        coroutine = co;
        return false;
    });
    return {action, nullptr};
}

coroutine::Awaiter_void Sleepfor(int64_t ms)
{
    auto func = [ms](coroutine::Coroutine* co, void* ctx)->bool{
        if(ms <= 0) return false;
        auto timecb = [co](details::TimeEvent::wptr event, Timer::ptr timer){
            co->BelongScheduler()->EnqueueCoroutine(co);
        };
        auto timer = std::make_shared<Timer>(ms, std::move(timecb));
        auto coexitcb = [timer]() {
            timer->Cancle();
        };
        co->AppendExitCallback(coexitcb);
        TimerSchedulerHolder::GetInstance()->GetScheduler(0)->AddTimer(ms,  timer);
        return true;
    };
    auto* action = new details::CoroutineHandleAction(std::move(func));
    return {action , nullptr};
}


coroutine::Awaiter_void DeferExit(const std::function<void(void)>& callback)
{
    auto* action = new details::CoroutineHandleAction([callback](coroutine::Coroutine* co, void* ctx)->bool{
        co->AppendExitCallback(std::move(callback));
        return false;
    });
    return {action, nullptr};
}


RoutineDeadLine::RoutineDeadLine(const std::function<void(void)>& callback)
    : m_callback(callback), m_last_active_time(GetCurrentTimeMs())
{
}

bool RoutineDeadLine::Refluash()
{
    uint64_t old = m_last_active_time.load();
    if(!m_last_active_time.compare_exchange_strong(old, GetCurrentTimeMs())) {
        return false;
    }
    return true;
}

// ToDo
coroutine::Awaiter_bool RoutineDeadLine::operator()(uint64_t ms)
{
}

}