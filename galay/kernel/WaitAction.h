#ifndef GALAY_WAITACTION_H
#define GALAY_WAITACTION_H

#include "Coroutine.hpp"

namespace galay::details
{
class WaitEvent: public Event
{
public:
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
    CoroutineBase::wptr m_waitco;
};

/*
    one net event be triggered will resume this coroutine
*/

class IOEventAction: public WaitAction
{
public:
    using ptr = std::shared_ptr<IOEventAction>;

    IOEventAction(EventEngine* engine, WaitEvent* event);
    bool HasEventToDo() override;
    // Add NetEvent to EventEngine
    bool DoAction(CoroutineBase::wptr co, void* ctx) override;
    void ResetEvent(details::WaitEvent* event);
    [[nodiscard]] details::WaitEvent* GetBindEvent() const { return m_event; };
    ~IOEventAction() override;
private:
    EventEngine* m_engine;
    WaitEvent* m_event;
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