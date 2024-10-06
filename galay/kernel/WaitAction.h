#ifndef __GALAY_AWAITERACTION_H__
#define __GALAY_AWAITERACTION_H__

#include "Awaiter.h"

namespace galay::event{
    class WaitEvent;
}

namespace galay::action {

/*
    one net event be triggered will resume this coroutine
*/
class NetIoEventAction: public WaitAction
{
public:
    NetIoEventAction();
    NetIoEventAction(event::WaitEvent* event, bool is_heap, ActionType type);
    virtual bool HasEventToDo() override;
    // Add NetEvent to EventEngine
    virtual bool DoAction(coroutine::Coroutine* co) override;
    virtual void ResetEvent(event::WaitEvent* event, bool is_heap) override;
    virtual void ResetActionType(ActionType type) override;
    virtual event::WaitEvent* GetBindEvent() override;
    virtual ~NetIoEventAction() = default;
private:
    bool m_is_heap;
    event::WaitEvent* m_event;
    ActionType m_type;
};

/*
    one coroutine done will resum this coroutine
*/
class CoroutineWaitAction: public WaitAction
{
public:
    CoroutineWaitAction();
    // return true
    virtual bool HasEventToDo() override;
    // Just Save Coroutine*
    virtual bool DoAction(coroutine::Coroutine* co) override;
    // Do nothing
    virtual void ResetEvent(event::WaitEvent* event, bool is_heap) override;
    // Do nothing
    virtual void ResetActionType(ActionType type) override;
    // return nullptr
    virtual event::WaitEvent* GetBindEvent() override;
    inline coroutine::Coroutine* GetCoroutine() { return m_coroutine; };
private:
    coroutine::Coroutine* m_coroutine;
};

}

#endif
