#include "Coroutine.hpp"
#include "Event.h"
#include "Scheduler.h"
#include "WaitAction.h"
#include "ExternApi.h"
#include "Log.h"

namespace galay
{
CoroutineStore::CoroutineStore()
{
    m_coroutines.reserve(DEFAULT_COROUTINE_STORE_HASH_SIZE);
}

void CoroutineStore::Reserve(size_t size)
{
    m_coroutines.reserve(size);
}

void CoroutineStore::AddCoroutine(std::weak_ptr<Coroutine> co)
{
    std::unique_lock lk(m_mutex);
    auto it = m_coroutines.emplace(co.lock().get(), co);
    if (!it.second) {
        LogError("[Add coroutine failed, coroutine: {}]", static_cast<void*>(co.lock().get()));
    }
}

void CoroutineStore::RemoveCoroutine(std::weak_ptr<Coroutine> co)
{
    std::unique_lock lk(m_mutex);
    auto it = m_coroutines.find(co.lock().get());
    if(it != m_coroutines.end()) {
        m_coroutines.erase(it);
    }
    else LogError("[Remove coroutine failed, coroutine: {}]", static_cast<void*>(co.lock().get()));
}

bool CoroutineStore::Exist(std::weak_ptr<Coroutine> co)
{
    std::shared_lock lk(m_mutex);
    return m_coroutines.contains(co.lock().get());
}


void CoroutineStore::Clear()
{
    std::unique_lock lk(m_mutex);
    while(!m_coroutines.empty()) {
        auto it = m_coroutines.begin();
        lk.unlock();
        CoroutineSchedulerHolder::GetInstance()->GetScheduler()->ToDestroyCoroutine(it->second);
        m_coroutines.erase(it); 
        lk.lock();
    }
}

Coroutine Coroutine::promise_type::get_return_object() noexcept
{
    m_coroutine = std::make_shared<Coroutine>(std::coroutine_handle<promise_type>::from_promise(*this), CoroutineSchedulerHolder::GetInstance()->GetScheduler());
    return *m_coroutine;
}

Coroutine::promise_type::~promise_type()
{
    //防止协程运行时resume创建出新的协程，析构时重复析构
    if(m_coroutine) {
        m_coroutine->ToExit();
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

bool Coroutine::SetAwaiter(AwaiterBase* awaiter)
{
    if(AwaiterBase* t = m_awaiter.load(); !m_awaiter.compare_exchange_strong(t, awaiter)) {
        return false;
    }
    return true;
}

void Coroutine::AppendExitCallback(const std::function<void()>& callback)
{
    m_exit_cbs.emplace_front(callback);
}

AwaiterBase *Coroutine::GetAwaiter() const
{
    return m_awaiter.load();
}

void Coroutine::ToExit()
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
        if(!m_coroutine.expired()) {
            static_cast<Awaiter<bool>*>(m_coroutine.lock()->GetAwaiter())->SetResult(true);
            m_coroutine.lock()->BelongScheduler()->ToResumeCoroutine(m_coroutine);
        }
    }
}

void WaitGroup::Reset(uint32_t count)
{
    std::unique_lock lk(m_mutex);
    m_count = count;
    m_coroutine = {};
}

Awaiter<bool> WaitGroup::Wait()
{
    auto* action = new details::CoroutineHandleAction([this](Coroutine::wptr co, void* ctx)->bool{
        std::unique_lock lk(m_mutex);
        if(m_count == 0) {
            if(!co.expired()) static_cast<Awaiter<bool>*>(co.lock()->GetAwaiter())->SetResult(false);
            return false;
        }
        m_coroutine = co;
        return true;
    });
    return {action, nullptr};
}

Awaiter<void> RoutineContext::DeferDone()
{
    auto* action = new details::CoroutineHandleAction([this](Coroutine::wptr co, void* ctx)->bool{
        m_coroutine = co;
        co.lock()->AppendExitCallback([this](){
            m_coroutine = {};
        });
        return false;
    });
    return {action, nullptr};
}

void RoutineContext::BeCanceled()
{
    if(!m_coroutine.expired()) {
        m_coroutine.lock()->BelongScheduler()->ToDestroyCoroutine(m_coroutine);
    }
}

void RoutineContextCancel::operator()()
{
    if(!m_context.expired()) m_context.lock()->BeCanceled();
}

Awaiter<bool> RoutineContextWithWaitGroup::Wait()
{
    return m_wait_group.Wait();
}

Awaiter<void> RoutineContextWithWaitGroup::DeferDone()
{
    auto* action = new details::CoroutineHandleAction([this](Coroutine::wptr co, void* ctx)->bool{
        m_coroutine = co;
        co.lock()->AppendExitCallback([this](){
            m_wait_group.Done();
        });
        co.lock()->AppendExitCallback([this](){
            m_coroutine = {};
        });
        return false;
    });
    return {action, nullptr};
}

void RoutineContextWithWaitGroup::BeCanceled()
{
    m_wait_group.Reset(0);
    if(!m_coroutine.expired()) {
        m_coroutine.lock()->BelongScheduler()->ToDestroyCoroutine(m_coroutine);
    }
}
}

namespace galay
{
CoroutineStore g_coroutine_store;

CoroutineStore* GetCoroutineStore()
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
Awaiter<void> AddToCoroutineStore()
{
    auto* action = new details::CoroutineHandleAction([](Coroutine::wptr co, void* ctx)->bool{
        co.lock()->AppendExitCallback([co](){
            GetCoroutineStore()->RemoveCoroutine(co);
        });
        GetCoroutineStore()->AddCoroutine(co);
        return false;
    });
    return {action, nullptr};
}

Awaiter<Coroutine::wptr> GetThisCoroutine()
{
    auto* action = new details::CoroutineHandleAction([](Coroutine::wptr co, void* ctx)->bool{
        static_cast<Awaiter<Coroutine::wptr>*>(co.lock()->GetAwaiter())->SetResult(co);
        return false;
    });
    return {action, nullptr};
}

Awaiter<void> Sleepfor(int64_t ms)
{
    auto func = [ms](Coroutine::wptr co, void* ctx)->bool{
        if(ms <= 0) return false;
        auto timecb = [co](details::TimeEvent::wptr event, Timer::ptr timer){
            if(!co.expired()) co.lock()->BelongScheduler()->ToResumeCoroutine(co);
        };
        auto timer = std::make_shared<Timer>(ms, std::move(timecb));
        auto coexitcb = [timer]() {
            timer->Cancle();
        };
        co.lock()->AppendExitCallback(coexitcb);
        TimerSchedulerHolder::GetInstance()->GetScheduler()->AddTimer(ms,  timer);
        return true;
    };
    auto* action = new details::CoroutineHandleAction(std::move(func));
    return {action , nullptr};
}


Awaiter<void> DeferExit(const std::function<void(void)>& callback)
{
    auto* action = new details::CoroutineHandleAction([callback](Coroutine::wptr co, void* ctx)->bool{
        co.lock()->AppendExitCallback(std::move(callback));
        return false;
    });
    return {action, nullptr};
}

AutoDestructor::AutoDestructor()
{
}

Awaiter<void> AutoDestructor::Start(uint64_t ms)
{
    auto weak_this = weak_from_this();
    auto func = [weak_this, ms](Coroutine::wptr co, void* ctx)->bool{
        auto timecb = [co, weak_this](details::TimeEvent::wptr event, Timer::ptr timer){
            if(timer->GetDeadline() > GetCurrentTimeMs() + MIN_REMAIN_TIME){
                if(!event.expired()) event.lock()->AddTimer(timer->GetDeadline() - GetCurrentTimeMs(),  timer);
            } else {
                if(!weak_this.expired() && weak_this.lock()->m_callback) weak_this.lock()->m_callback();
                if(!co.expired()) co.lock()->BelongScheduler()->ToDestroyCoroutine(co);
            }
        };
        auto timer = std::make_shared<Timer>(ms, std::move(timecb));
        weak_this.lock()->m_timer = timer;
        TimerSchedulerHolder::GetInstance()->GetScheduler()->AddTimer(ms,  weak_this.lock()->m_timer);
        return false;
    };
    auto* action = new details::CoroutineHandleAction(std::move(func));
    return {action, nullptr};
}

bool AutoDestructor::Reflush()
{
    m_timer->ResetTimeout(m_timer->GetTimeout());
    return false;
}

}