#ifndef GALAY_WAITACTION_H
#define GALAY_WAITACTION_H

#include "Coroutine.hpp"
#include "Time.hpp"

namespace galay::details
{
class WaitEvent: public Event
{
public:
    using uptr = std::unique_ptr<WaitEvent>;
    WaitEvent();
    /*
        OnWaitPrepare() return false coroutine will not suspend, else suspend
    */
    virtual bool OnWaitPrepare(CoroutineBase::wptr co, void* ctx) = 0;
    bool SetEventEngine(EventEngine* engine) override;
    EventEngine* BelongEngine() override;
    ~WaitEvent() override = default;
protected:
    std::atomic<EventEngine*> m_engine;
};

/*
    one net event be triggered will resume this coroutine
*/

class IOEventAction: public WaitAction
{
public:
    using uptr = std::unique_ptr<IOEventAction>;
    using ptr = std::shared_ptr<IOEventAction>;

    IOEventAction(WaitEvent::uptr event);
    bool HasEventToDo() override;
    // Add NetEvent to EventEngine
    bool DoAction(CoroutineBase::wptr co, void* ctx) override;
    details::WaitEvent* GetBindEvent() const { return m_event.get(); };
private:
    WaitEvent::uptr m_event;
};


class CoroutineHandleAction: public WaitAction
{
public:
    CoroutineHandleAction(std::function<bool(CoroutineBase::wptr, void*)>&& callback);
    virtual bool HasEventToDo() override;
    virtual bool DoAction(CoroutineBase::wptr co, void* ctx) override;
private:
    std::function<bool(CoroutineBase::wptr, void*)> m_callback;
};


}


#endif