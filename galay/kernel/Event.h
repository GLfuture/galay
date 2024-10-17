#ifndef __GALAY_EVENT_H__
#define __GALAY_EVENT_H__

#include "Base.h"
#include <any>
#include <queue>
#include <mutex>
#include <atomic>
#include <memory>
#include <functional>
#include <shared_mutex>

namespace galay::coroutine
{
    class Coroutine;
}

namespace galay::async
{
    class AsyncTcpSocket;
}

namespace galay::scheduler
{
    class EventScheduler;
    class CoroutineScheduler;
}

namespace galay
{
    class CallbackStore;
}

namespace galay::event
{
#define DEFAULT_READ_BUFFER_SIZE    1024

enum EventType
{
    kEventTypeRead,
    kEventTypeWrite,
};

class EventEngine;
class Event;

//must be alloc at heap
class Event
{
public:
    /*
        HandleEvent() will be called in EventEngine::Loop(),
        you should call SetEventType() to reentry the event to ready list or sleep list or delete it
    */
    virtual std::string Name() = 0;
    virtual void HandleEvent(EventEngine* engine) = 0;
    virtual void SetEventType(EventType type) = 0;
    virtual EventType GetEventType() = 0;
    virtual GHandle GetHandle() = 0;
    virtual void Free(EventEngine* engine) = 0;
};

class CallbackEvent: public Event
{
public:
    CallbackEvent(GHandle handle, EventType type, std::function<void(Event*, EventEngine*)> callback);
    virtual void HandleEvent(EventEngine* engine) override;
    inline virtual std::string Name() override { return "CallbackEvent"; }
    inline virtual void SetEventType(EventType type) override { m_type = type; }
    inline virtual EventType GetEventType() override { return m_type; }
    inline virtual GHandle GetHandle() override { return m_handle; }
    virtual void Free(EventEngine* engine) override;
private:
    EventType m_type;
    GHandle m_handle;
    std::function<void(Event*, EventEngine*)> m_callback;
};

class CoroutineEvent: public Event
{
public:
    CoroutineEvent(GHandle handle, EventEngine* engine, EventType type);
    virtual void HandleEvent(EventEngine* engine) override;
    inline virtual std::string Name() override { return "CoroutineEvent"; }
    inline virtual void SetEventType(EventType type) override { m_type = type; }
    inline virtual EventType GetEventType() override { return m_type; }
    inline virtual GHandle GetHandle() override { return m_handle; }
    virtual void Free(EventEngine* engine) override;
    void ResumeCoroutine(coroutine::Coroutine* coroutine);
    
private:
    std::shared_mutex m_mtx;
    GHandle m_handle;
    EventType m_type;
    EventEngine* m_engine;
    std::queue<coroutine::Coroutine*> m_coroutines;
    
};

class ListenEvent: public Event
{
public:
    ListenEvent(GHandle handle, CallbackStore* callback_store \
        , scheduler::EventScheduler* net_scheduler, scheduler::CoroutineScheduler* co_scheduler);
    virtual void HandleEvent(EventEngine* engine) override;
    inline virtual std::string Name() override { return "ListenEvent"; }
    inline virtual void SetEventType(EventType type) override {}
    inline virtual EventType GetEventType() override { return kEventTypeRead; }
    inline virtual GHandle GetHandle() override { return m_handle; }
    virtual void Free(EventEngine* engine) override;
private:
    coroutine::Coroutine CreateTcpSocket(EventEngine* engine);
private:
    GHandle m_handle;
    scheduler::EventScheduler* m_net_scheduler;
    scheduler::CoroutineScheduler* m_co_scheduler;
    CallbackStore* m_callback_store;
};

class TimeEvent: public Event
{
public:
    /*
        if timer is cancled and callback is not executed, Success while return false
    */
    class Timer: public std::enable_shared_from_this<Timer> 
    {
        friend class TimeEvent;
    public:
        using ptr = std::shared_ptr<Timer>;
        class TimerCompare
        {
        public:
            bool operator()(const Timer::ptr &a, const Timer::ptr &b);
        };
        Timer(int64_t during_time, std::function<void(Timer::ptr)> &&func);
        int64_t GetDuringTime();  
        int64_t GetExpiredTime();
        int64_t GetRemainTime();
        inline std::any& GetContext() { return m_context; };
        void SetDuringTime(int64_t during_time);
        void Execute();
        void Cancle();
        bool Success();
    private:
        std::any m_context;
        int64_t m_expiredTime;
        int64_t m_duringTime;
        std::atomic_bool m_cancle;
        std::atomic_bool m_success;
        std::function<void(Timer::ptr)> m_rightHandle;
    };
    TimeEvent(GHandle handle);
    virtual std::string Name() override { return "TimeEvent"; };
    virtual void HandleEvent(EventEngine* engine) override;
    inline virtual void SetEventType(EventType type) override {};
    inline virtual EventType GetEventType() override { return kEventTypeRead; };
    inline virtual GHandle GetHandle() override { return m_handle; }
    virtual void Free(EventEngine* engine) override;
    Timer::ptr AddTimer(int64_t during_time, std::function<void(Timer::ptr)> &&func); // ms
    void ReAddTimer(int64_t during_time, Timer::ptr timer);
private:
    void UpdateTimers();
private:
    GHandle m_handle;
    std::shared_mutex m_mutex;
    std::priority_queue<Timer::ptr> m_timers;
};

class WaitEvent: public Event
{
public:
    WaitEvent(event::EventEngine* engine, async::AsyncTcpSocket* socket, EventType type);
    inline virtual std::string Name() override { return "WaitEvent"; }
    virtual void HandleEvent(EventEngine* engine) override;
    inline virtual void SetEventType(EventType type) override { m_type = type; }
    inline virtual EventType GetEventType() override { return m_type; }
    virtual void Free(EventEngine* engine) override;
    virtual GHandle GetHandle() override;
    inline async::AsyncTcpSocket* GetAsyncTcpSocket() { return m_socket; }
    inline event::EventEngine* GetEventEngine() { return m_engine; }

    void SetWaitCoroutine(coroutine::Coroutine* co);
    virtual ~WaitEvent() = default;
protected:
    EventType m_type;
    event::EventEngine* m_engine;
    async::AsyncTcpSocket* m_socket;
    coroutine::Coroutine* m_waitco;
};

class RecvEvent: public WaitEvent
{
public:
    RecvEvent(event::EventEngine* engine, async::AsyncTcpSocket* socket);
    inline virtual std::string Name() override { return "RecvEvent"; }
    virtual void HandleEvent(EventEngine* engine) override;
    virtual ~RecvEvent() = default;
};

class SendEvent: public WaitEvent
{
public:
    SendEvent(event::EventEngine* engine, async::AsyncTcpSocket* socket);
    inline virtual std::string Name() override { return "SendEvent"; }
    virtual void HandleEvent(EventEngine* engine) override;
    virtual ~SendEvent() = default;
};
    
}
#endif