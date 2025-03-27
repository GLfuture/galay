#include "WaitAction.h"

namespace galay::details
{



WaitEvent::WaitEvent()
    :m_engine(nullptr)
{
}


bool WaitEvent::SetEventEngine(EventEngine *engine)
{
    if(details::EventEngine* t = m_engine.load(); !m_engine.compare_exchange_strong(t, engine)) {
        return false;
    }
    return true;
}


EventEngine* WaitEvent::BelongEngine()
{
    return m_engine.load();
}


IOEventAction::IOEventAction(WaitEvent::uptr event)
    :m_event(std::move(event))
{
}


bool IOEventAction::HasEventToDo()
{
    return m_event != nullptr;
}


bool IOEventAction::DoAction(CoroutineBase::wptr co, void* ctx)
{
    if( !m_event ) return false;
    if (m_event->OnWaitPrepare(co, ctx) == false) return false;
    EventEngine* engine = co.lock()->GetEventScheduler()->GetEngine();
    if (!m_event->BelongEngine())   {
        if(const int ret = engine->AddEvent(this->m_event.get(), nullptr); ret != 0 ) {
            LogWarn("[Add handle({}) to engine:{} failed, error: {}]", m_event->GetHandle().fd, (void*)m_event->BelongEngine(), error::GetErrorString(engine->GetErrorCode()));
            m_event->BelongEngine()->ModEvent(this->m_event.get(), nullptr);
            return true;
        }
    }
    else {
        if(const int ret = m_event->BelongEngine()->ModEvent(this->m_event.get(), nullptr); ret != 0 ) {
            LogWarn("[Mod handle({}) from engine:{} failed, error: {}]", m_event->GetHandle().fd, (void*)m_event->BelongEngine(), error::GetErrorString(engine->GetErrorCode()));
            m_event->BelongEngine()->AddEvent(this->m_event.get(), nullptr);
            return true;
        }
    }
    return true;
}


CoroutineHandleAction::CoroutineHandleAction(std::function<bool(CoroutineBase::wptr, void*)>&& callback)
    : m_callback(std::move(callback))
{
    
}


bool CoroutineHandleAction::HasEventToDo()
{
    return true;
}


bool CoroutineHandleAction::DoAction(CoroutineBase::wptr co, void* ctx)
{
    bool res = m_callback(co, ctx);
    delete this;
    return res;
}


}