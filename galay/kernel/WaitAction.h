#ifndef __GALAY_AWAITERACTION_H__
#define __GALAY_AWAITERACTION_H__

#include "Awaiter.h"

namespace galay::event{
    class NetWaitEvent;
}

namespace galay::action {

/*
    one net event be triggered will resume this coroutine
*/
class NetIoEventAction: public WaitAction
{
public:
    using ptr = std::shared_ptr<NetIoEventAction>;

    NetIoEventAction();
    NetIoEventAction(event::NetWaitEvent* event);
    virtual bool HasEventToDo() override;
    // Add NetEvent to EventEngine
    virtual bool DoAction(coroutine::Coroutine* co) override;
    void ResetEvent(event::NetWaitEvent* event);
    event::NetWaitEvent* GetBindEvent();
    virtual ~NetIoEventAction() = default;
private:
    event::NetWaitEvent* m_event;
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
