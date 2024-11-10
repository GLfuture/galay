#include "WaitAction.h"
#include "Event.h"
#include "Async.h"
#include "EventEngine.h"
#include <spdlog/spdlog.h>

namespace galay::action
{

TcpEventAction::TcpEventAction(event::EventEngine* engine)
    :m_engine(engine), m_event(nullptr)
{
    
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
    /*
        MultiThread environment, EventInEngine may be incorrect, so we need to call ModEvent/AddEvent
        after AddEvent/ModEvent failed. 
    */
    if( !m_event->BelongEngine() ){
        int ret = m_engine->AddEvent(this->m_event);
        if(  ret != 0 ) {
            spdlog::warn("TcpEventAction::DoAction.AddEvent(handle: {}) failed, {}", m_event->GetAsyncTcpSocket()->GetHandle().fd, error::GetErrorString(m_engine->GetErrorCode()));
            m_engine->ModEvent(this->m_event);
            return true;
        } 
    } else {
        int ret = m_engine->ModEvent(this->m_event);
        if( ret != 0 ) {
            spdlog::warn("TcpEventAction::DoAction.ModEvent(handle: {}) failed, {}", m_event->GetAsyncTcpSocket()->GetHandle().fd, error::GetErrorString(m_engine->GetErrorCode()));
            m_engine->AddEvent(this->m_event);
            return true;
        }
    }
    return true;
}

void TcpEventAction::ResetEvent(event::TcpWaitEvent *event)
{
    this->m_event = event;
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