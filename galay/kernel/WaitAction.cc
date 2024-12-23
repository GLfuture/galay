#include "WaitAction.h"
#include "Event.h"
#include "Async.h"
#include "EventEngine.h"
#include "Scheduler.h"
#include "ExternApi.h"
#include "Time.h"
#include "Log.h"

namespace galay::details
{

IOEventAction::IOEventAction(details::EventEngine* engine, details::WaitEvent *event)
    :m_engine(engine), m_event(event)
{
}

bool IOEventAction::HasEventToDo()
{
    return m_event != nullptr;
}

bool IOEventAction::DoAction(Coroutine::wptr co, void* ctx)
{
    if( !m_event ) return false;
    if (m_event->OnWaitPrepare(co, ctx) == false) return false;
    if (!m_event->BelongEngine())   {
        if(const int ret = m_engine->AddEvent(this->m_event, nullptr); ret != 0 ) {
            LogWarn("[Add handle({}) to engine:{} failed, error: {}]", m_event->GetHandle().fd, (void*)m_event->BelongEngine(), error::GetErrorString(m_engine->GetErrorCode()));
            m_event->BelongEngine()->ModEvent(this->m_event, nullptr);
            return true;
        }
    }
    else {
        if(const int ret = m_event->BelongEngine()->ModEvent(this->m_event, nullptr); ret != 0 ) {
            LogWarn("[Mod handle({}) from engine:{} failed, error: {}]", m_event->GetHandle().fd, (void*)m_event->BelongEngine(), error::GetErrorString(m_engine->GetErrorCode()));
            m_event->BelongEngine()->AddEvent(this->m_event, nullptr);
            return true;
        }
    }
    return true;
}

void IOEventAction::ResetEvent(details::WaitEvent *event)
{
    this->m_event = event;
}

IOEventAction::~IOEventAction()
{
    delete m_event;
}


CoroutineHandleAction::CoroutineHandleAction(std::function<bool(Coroutine::wptr, void*)>&& callback)
    : m_callback(std::forward<std::function<bool(Coroutine::wptr, void*)>>(callback))
{
    
}

bool CoroutineHandleAction::HasEventToDo()
{
    return true;
}

bool CoroutineHandleAction::DoAction(Coroutine::wptr co, void* ctx)
{
    bool res = m_callback(co, ctx);
    delete this;
    return res;
}


}