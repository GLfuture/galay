#ifndef GALAY_WAITACTION_H
#define GALAY_WAITACTION_H

#include "Coroutine.hpp"
#include <functional>

namespace galay::details {

class WaitEvent;
class EventEngine;

/*
    one net event be triggered will resume this coroutine
*/
class IOEventAction: public WaitAction
{
public:
    using ptr = std::shared_ptr<IOEventAction>;

    IOEventAction(details::EventEngine* engine, details::WaitEvent* event);
    bool HasEventToDo() override;
    // Add NetEvent to EventEngine
    bool DoAction(Coroutine::wptr co, void* ctx) override;
    void ResetEvent(details::WaitEvent* event);
    [[nodiscard]] details::WaitEvent* GetBindEvent() const { return m_event; };
    ~IOEventAction() override;
private:
    details::EventEngine* m_engine;
    details::WaitEvent* m_event;
};


class CoroutineHandleAction: public WaitAction
{
public:
    CoroutineHandleAction(std::function<bool(Coroutine::wptr, void*)>&& callback);
    virtual bool HasEventToDo() override;
    virtual bool DoAction(Coroutine::wptr co, void* ctx) override;
private:
    std::function<bool(Coroutine::wptr, void*)> m_callback;
};

}

#endif
