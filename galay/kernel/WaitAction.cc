#include "WaitAction.h"
#include "Event.h"
#include "Async.h"
#include "EventEngine.h"
#include <spdlog/spdlog.h>

namespace galay::action
{

NetIoEventAction::NetIoEventAction()
    :m_event(nullptr)
{
    
}

NetIoEventAction::NetIoEventAction(event::NetWaitEvent *event)
{
    this->m_event = event;
}

bool NetIoEventAction::HasEventToDo()
{
    return m_event != nullptr;
}

bool NetIoEventAction::DoAction(coroutine::Coroutine *co)
{
    if( !m_event ) return false;
    if (m_event->OnWaitPrepare(co) == false) return false;
    event::EventEngine* engine = m_event->GetEventEngine();
    if( !m_event->EventInEngine() ){
        if( engine->AddEvent(this->m_event) != 0 ) {
            spdlog::error("NetIoEventAction::DoAction.AddEvent failed, {}", engine->GetLastError());
            return false;
        }
    } else {
        if( engine->ModEvent(this->m_event) != 0 ) {
            spdlog::error("NetIoEventAction::DoAction.ModEvent failed, {}", engine->GetLastError());
            return false;
        }
    }
    return true;
}

void NetIoEventAction::ResetEvent(event::NetWaitEvent *event)
{
    this->m_event = event;
}

event::NetWaitEvent *NetIoEventAction::GetBindEvent()
{
    return m_event;
}

CoroutineWaitAction::CoroutineWaitAction()
{
    this->m_coroutine = nullptr;
}

bool CoroutineWaitAction::HasEventToDo()
{
    return true;
}

bool CoroutineWaitAction::DoAction(coroutine::Coroutine *co)
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

bool GetCoroutineHandleAction::DoAction(coroutine::Coroutine *co)
{
    *(this->m_coroutine) = co;
    delete this;
    return false;
}

}