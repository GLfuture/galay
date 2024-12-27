#include "Internal.hpp"
#include "EventEngine.h"


namespace galay::details
{

std::string ToString(const EventType type)
{
    switch (type)
    {
    case kEventTypeNone:
        return "EventTypeNone";
    case kEventTypeRead:
        return "EventTypeRead";
    case kEventTypeWrite:
        return "EventTypeWrite";
    case kEventTypeTimer:
        return "EventTypeTimer";
    case kEventTypeError:
        return "EventTypeError";
    default:
        break;
    }
    return ""; 
}

CallbackEvent::CallbackEvent(const GHandle handle, const EventType type, std::function<void(Event*, EventEngine*)> callback)
    : m_type(type), m_handle(handle), m_engine(nullptr), m_callback(std::move(callback))
{
    
}

void CallbackEvent::HandleEvent(EventEngine *engine)
{
    this->m_callback(this, engine);
}

bool CallbackEvent::SetEventEngine(EventEngine *engine)
{
    if(details::EventEngine* t = m_engine.load(); !m_engine.compare_exchange_strong(t, engine)) {
        return false;
    }
    return true;
}

EventEngine* CallbackEvent::BelongEngine()
{
    return m_engine.load();
}


CallbackEvent::~CallbackEvent()
{
    if( m_engine ) m_engine.load()->DelEvent(this, nullptr);
}

}