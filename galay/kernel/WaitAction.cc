#include "WaitAction.h"
#include "Event.h"
#include "Async.h"
#include "EventEngine.h"
#include <spdlog/spdlog.h>

namespace galay::action
{

NetIoEventAction::NetIoEventAction()
    :m_event(nullptr), m_type(kActionToAddEvent)
{
    
}

NetIoEventAction::NetIoEventAction(event::WaitEvent *event, ActionType type)
{
    this->m_event = event;
    this->m_type = type;
}

bool NetIoEventAction::HasEventToDo()
{
    return m_event != nullptr;
}

bool NetIoEventAction::DoAction(coroutine::Coroutine *co)
{
    if( !m_event ) return false;
    m_event->SetWaitCoroutine(co);
    event::EventEngine* engine = m_event->GetEventEngine();
    switch (m_type)
    {
    case kActionToAddEvent:
    {
        if( engine->AddEvent(this->m_event) != 0 ) {
            spdlog::error("NetIoEventAction::DoAction.AddEvent failed, {}", engine->GetLastError());
            return false;
        }
    }
        break;
    case kActionToModEvent:
    {
        if( engine->ModEvent(this->m_event) != 0 ) {
            spdlog::error("NetIoEventAction::DoAction.ModEvent failed, {}", engine->GetLastError());
            return false;
        }
    }
        break;
    case kActionToDelEvent:
    {
        if( engine->DelEvent(this->m_event) != 0 ) {
            spdlog::error("NetIoEventAction::DoAction.DelEvent failed, {}", engine->GetLastError());
            return false;
        }
    }
        break;
    }
    return true;
}

void NetIoEventAction::ResetEvent(event::WaitEvent *event)
{
    this->m_event = event;
}

void NetIoEventAction::ResetActionType(ActionType type)
{
    this->m_type = type;
}

event::WaitEvent *NetIoEventAction::GetBindEvent()
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