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
    enum ActionType
    {
        kActionToAddEvent,
        kActionToModEvent,
        kActionToDelEvent,
    };
    using ptr = std::shared_ptr<NetIoEventAction>;

    NetIoEventAction();
    NetIoEventAction(event::WaitEvent* event, ActionType type);
    virtual bool HasEventToDo() override;
    // Add NetEvent to EventEngine
    virtual bool DoAction(coroutine::Coroutine* co) override;
    void ResetEvent(event::WaitEvent* event);
    void ResetActionType(ActionType type);
    event::WaitEvent* GetBindEvent();
    virtual ~NetIoEventAction() = default;
private:
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
    inline coroutine::Coroutine* GetCoroutine() { return m_coroutine; };
private:
    coroutine::Coroutine* m_coroutine;
};

class GetCoroutineHandleAction: public WaitAction
{
public:
    GetCoroutineHandleAction(coroutine::Coroutine** m_coroutine);
    virtual bool HasEventToDo() override;
    virtual bool DoAction(coroutine::Coroutine* co) override;
    inline coroutine::Coroutine* GetCoroutine() { return *m_coroutine; };
private:
    coroutine::Coroutine** m_coroutine;
};

}

#endif
