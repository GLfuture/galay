#ifndef __GALAY_AWAITERACTION_H__
#define __GALAY_AWAITERACTION_H__

#include "Awaiter.h"
#include <functional>

namespace galay::event{
    class NetWaitEvent;
    class NetSslWaitEvent;
    class UdpWaitEvent;
    class FileIoWaitEvent;
    class EventEngine;
    class Timer;
}

namespace galay::action {

/*
    global 
*/
class TimeEventAction: public WaitAction
{
public:
    using ptr = std::shared_ptr<TimeEventAction>;
    TimeEventAction();
    void CreateTimer(int64_t ms, std::shared_ptr<event::Timer>* timer, std::function<void(const std::shared_ptr<event::Timer>&)>&& callback);
    bool HasEventToDo() override;
    // Add Timer
    bool DoAction(coroutine::Coroutine* co, void* ctx) override;
    ~TimeEventAction() override;
private:
    int64_t m_ms{};
    std::shared_ptr<event::Timer>* m_timer{};
    std::function<void(const std::shared_ptr<event::Timer>&)> m_callback;
};

/*
    one net event be triggered will resume this coroutine
*/
class IOEventAction: public WaitAction
{
public:
    using ptr = std::shared_ptr<IOEventAction>;

    IOEventAction(event::EventEngine* engine, event::WaitEvent* event);
    bool HasEventToDo() override;
    // Add NetEvent to EventEngine
    bool DoAction(coroutine::Coroutine* co, void* ctx) override;
    void ResetEvent(event::WaitEvent* event);
    [[nodiscard]] event::WaitEvent* GetBindEvent() const { return m_event; };
    ~IOEventAction() override;
private:
    event::EventEngine* m_engine;
    event::WaitEvent* m_event;
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
