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
class NetEventAction: public WaitAction
{
public:
    using ptr = std::shared_ptr<NetEventAction>;

    NetEventAction(event::EventEngine* engine, event::NetWaitEvent* event);
    bool HasEventToDo() override;
    // Add NetEvent to EventEngine
    bool DoAction(coroutine::Coroutine* co, void* ctx) override;
    void ResetEvent(event::NetWaitEvent* event);
    [[nodiscard]] event::NetWaitEvent* GetBindEvent() const { return m_event; };
    ~NetEventAction() override;
private:
    event::EventEngine* m_engine;
    event::NetWaitEvent* m_event;
};

class SslNetEventAction: public NetEventAction
{
public:
    using ptr = std::shared_ptr<SslNetEventAction>;
    SslNetEventAction(event::EventEngine* engine, event::NetSslWaitEvent* event);
};


class FileIoEventAction: public WaitAction
{
public:
    FileIoEventAction(event::EventEngine* engine, event::FileIoWaitEvent* event);
    virtual bool HasEventToDo() override;
    virtual bool DoAction(coroutine::Coroutine* co, void* ctx) override;
    inline event::FileIoWaitEvent* GetBindEvent() { return m_event; };
    virtual ~FileIoEventAction();
private:
    event::FileIoWaitEvent* m_event;
    event::EventEngine* m_engine;
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
