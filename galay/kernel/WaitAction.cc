#include "WaitAction.h"
#include "Event.h"
#include "Async.h"
#include "EventEngine.h"
#include "Scheduler.h"
#include "ExternApi.h"
#include <spdlog/spdlog.h>

namespace galay::action
{
void TimeEventAction::CreateTimer(int64_t ms, std::shared_ptr<event::Timer>* timer, std::function<void(std::shared_ptr<event::Timer>)> &&callback)
{
    this->m_ms = ms;
    m_callback = std::forward<std::function<void(std::shared_ptr<event::Timer>)>>(callback);
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

TcpEventAction::TcpEventAction(event::EventEngine* engine, event::TcpWaitEvent *event)
    :m_engine(engine), m_event(event)
{
}

bool TcpEventAction::HasEventToDo()
{
    return m_event != nullptr;
}

bool TcpEventAction::DoAction(coroutine::Coroutine *co, void* ctx)
{
    if( !m_event ) return false;
    if (m_event->OnWaitPrepare(co, ctx) == false) return false;
    if (!m_event->BelongEngine())   {
        int ret = m_engine->AddEvent(this->m_event, nullptr);
        if( ret != 0 ) {
            spdlog::warn("TcpEventAction::DoAction.AddEvent(handle: {}) failed, {}", m_event->GetAsyncTcpSocket()->GetHandle().fd, error::GetErrorString(m_event->BelongEngine()->GetErrorCode()));
            m_event->BelongEngine()->ModEvent(this->m_event, nullptr);
            return true;
        }
    }
    else {
        int ret = m_event->BelongEngine()->ModEvent(this->m_event, nullptr);
        if( ret != 0 ) {
            spdlog::warn("TcpEventAction::DoAction.ModEvent(handle: {}) failed, {}", m_event->GetAsyncTcpSocket()->GetHandle().fd, error::GetErrorString(m_event->BelongEngine()->GetErrorCode()));
            m_event->BelongEngine()->AddEvent(this->m_event, nullptr);
            return true;
        }
    }
    return true;
}

void TcpEventAction::ResetEvent(event::TcpWaitEvent *event)
{
    this->m_event = event;
}

TcpEventAction::~TcpEventAction()
{
    delete m_event;
}

event::TcpWaitEvent *TcpEventAction::GetBindEvent()
{
    return m_event;
}

TcpSslEventAction::TcpSslEventAction(event::EventEngine* engine, event::TcpSslWaitEvent * event)
    :TcpEventAction(engine, event)
{
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