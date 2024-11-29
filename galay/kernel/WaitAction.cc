#include "WaitAction.h"
#include "Event.h"
#include "Async.h"
#include "EventEngine.h"
#include "Scheduler.h"
#include "ExternApi.h"
#include <spdlog/spdlog.h>

namespace galay::action
{

TimeEventAction::TimeEventAction()
= default;

void TimeEventAction::CreateTimer(const int64_t ms, std::shared_ptr<event::Timer> *timer, std::function<void(const std::shared_ptr<event::Timer>&)> &&callback)
{
    this->m_ms = ms;
    m_callback = std::forward<std::function<void(const std::shared_ptr<event::Timer>&)>>(callback);
    m_timer = timer;
}

bool TimeEventAction::HasEventToDo()
{
    return true;
}


bool TimeEventAction::DoAction(coroutine::Coroutine *co, void *ctx)
{
    if(m_ms <= 0) {
        return false;
    } 
    *m_timer = GetTimerSchedulerInOrder()->AddTimer(m_ms, std::move(m_callback));
    (*m_timer)->GetContext() = co;
    return true;
}

TimeEventAction::~TimeEventAction()
= default;

IOEventAction::IOEventAction(event::EventEngine* engine, event::WaitEvent *event)
    :m_engine(engine), m_event(event)
{
}

bool IOEventAction::HasEventToDo()
{
    return m_event != nullptr;
}

bool IOEventAction::DoAction(coroutine::Coroutine *co, void* ctx)
{
    if( !m_event ) return false;
    if (m_event->OnWaitPrepare(co, ctx) == false) return false;
    if (!m_event->BelongEngine())   {
        if(const int ret = m_engine->AddEvent(this->m_event, nullptr); ret != 0 ) {
            spdlog::warn("IOEventAction::DoAction.AddEvent(handle: {}) failed, {}, engine:{}", m_event->GetHandle().fd, error::GetErrorString(m_engine->GetErrorCode()), (void*)m_event->BelongEngine());
            m_event->BelongEngine()->ModEvent(this->m_event, nullptr);
            return true;
        }
    }
    else {
        if(const int ret = m_event->BelongEngine()->ModEvent(this->m_event, nullptr); ret != 0 ) {
            spdlog::warn("IOEventAction::DoAction.ModEvent(handle: {}) failed, {}, engine:{}", m_event->GetHandle().fd, error::GetErrorString(m_engine->GetErrorCode()), (void*)m_event->BelongEngine());
            m_event->BelongEngine()->AddEvent(this->m_event, nullptr);
            return true;
        }
    }
    return true;
}

void IOEventAction::ResetEvent(event::WaitEvent *event)
{
    this->m_event = event;
}

IOEventAction::~IOEventAction()
{
    delete m_event;
}

CoroutineWaitAction::CoroutineWaitAction()
{
    this->m_coroutine = nullptr;
}

bool CoroutineWaitAction::HasEventToDo()
{
    return true;
}

bool CoroutineWaitAction::DoAction(coroutine::Coroutine *co, void* ctx)
{
    this->m_coroutine = co;
    return true;
}

GetCoroutineHandleAction::GetCoroutineHandleAction(coroutine::Coroutine** m_coroutine)
{
    this->m_coroutine = m_coroutine;
}

bool GetCoroutineHandleAction::HasEventToDo()
{
    return true;
}

bool GetCoroutineHandleAction::DoAction(coroutine::Coroutine *co, void* ctx)
{
    *(this->m_coroutine) = co;
    delete this;
    return false;
}


}