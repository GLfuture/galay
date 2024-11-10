#ifndef __GALAY_AWAITERACTION_H__
#define __GALAY_AWAITERACTION_H__

#include "Awaiter.h"

namespace galay::event{
    class TcpWaitEvent;
    class TcpSslWaitEvent;
    class UdpWaitEvent;
    class EventEngine;
}

namespace galay::action {

/*
    one net event be triggered will resume this coroutine
*/
class TcpEventAction: public WaitAction
{
public:
    using ptr = std::shared_ptr<TcpEventAction>;

    TcpEventAction(event::EventEngine* engine);
    TcpEventAction(event::EventEngine* engine, event::TcpWaitEvent* event);
    virtual bool HasEventToDo() override;
    // Add NetEvent to EventEngine
    virtual bool DoAction(coroutine::Coroutine* co, void* ctx) override;
    void ResetEvent(event::TcpWaitEvent* event);
    event::TcpWaitEvent* GetBindEvent();
    virtual ~TcpEventAction() = default;
private:
    event::EventEngine* m_engine;
    event::TcpWaitEvent* m_event;
};

class TcpSslEventAction: public TcpEventAction
{
public:
    using ptr = std::shared_ptr<TcpSslEventAction>;
    TcpSslEventAction(event::EventEngine* engine, event::TcpSslWaitEvent* event);
};

class UdpEventAction
{
public:
    UdpEventAction(event::UdpWaitEvent* event);
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
    virtual bool DoAction(coroutine::Coroutine* co, void* ctx) override;
    inline coroutine::Coroutine* GetCoroutine() { return m_coroutine; };
private:
    coroutine::Coroutine* m_coroutine;
};

class GetCoroutineHandleAction: public WaitAction
{
public:
    GetCoroutineHandleAction(coroutine::Coroutine** m_coroutine);
    virtual bool HasEventToDo() override;
    virtual bool DoAction(coroutine::Coroutine* co, void* ctx) override;
    inline coroutine::Coroutine* GetCoroutine() { return *m_coroutine; };
private:
    coroutine::Coroutine** m_coroutine;
};

}

#endif
