#ifndef __GALAY_AWAITERACTION_H__
#define __GALAY_AWAITERACTION_H__

#include "Awaiter.h"
#include <functional>

namespace galay::event{
    class TcpWaitEvent;
    class TcpSslWaitEvent;
    class UdpWaitEvent;
    class EventEngine;
    class Timer;
}

namespace galay::action {

class TimeEventAction: public WaitAction
{
public:
    using ptr = std::shared_ptr<TimeEventAction>;
    TimeEventAction() = default;
    void CreateTimer(int64_t ms, std::shared_ptr<event::Timer>* timer, std::function<void(std::shared_ptr<event::Timer>)>&& callback);
    virtual bool HasEventToDo() override;
    // Add Timer
    virtual bool DoAction(coroutine::Coroutine* co, void* ctx) override;
private:
    int64_t m_ms;
    std::shared_ptr<event::Timer>* m_timer;
    std::function<void(std::shared_ptr<event::Timer>)> m_callback;
};

/*
    one net event be triggered will resume this coroutine
*/
class TcpEventAction: public WaitAction
{
public:
    using ptr = std::shared_ptr<TcpEventAction>;

    TcpEventAction(event::EventEngine* engine, event::TcpWaitEvent* event);
    virtual bool HasEventToDo() override;
    // Add NetEvent to EventEngine
    virtual bool DoAction(coroutine::Coroutine* co, void* ctx) override;
    void ResetEvent(event::TcpWaitEvent* event);
    event::TcpWaitEvent* GetBindEvent();
    virtual ~TcpEventAction();
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
