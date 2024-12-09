#ifndef __GALAY_AWAITERACTION_H__
#define __GALAY_AWAITERACTION_H__

#include "Awaiter.h"
#include <functional>

namespace galay::details {

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
    bool DoAction(coroutine::Coroutine::wptr co, void* ctx) override;
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
    CoroutineHandleAction(std::function<bool(coroutine::Coroutine::wptr, void*)>&& callback);
    virtual bool HasEventToDo() override;
    virtual bool DoAction(coroutine::Coroutine::wptr co, void* ctx) override;
private:
    std::function<bool(coroutine::Coroutine::wptr, void*)> m_callback;
};

}

#endif
