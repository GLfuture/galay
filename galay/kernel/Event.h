#ifndef __GALAY_EVENT_H__
#define __GALAY_EVENT_H__

#include "../util/ThreadSecurity.hpp"
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

namespace galay::event
{
#define DEFAULT_READ_BUFFER_SIZE    1024

enum EventType
{
    kEventTypeRead,
    kEventTypeWrite,
};

class EventEngine;
class Strategy;
class Event;

class EventPosition
{
public:
    static EventPosition npos;
    EventPosition();
    EventPosition(thread::security::SecurityList<Event*>::ListNodePos pos);
    inline thread::security::SecurityList<Event*>::ListNodePos GetListPos() { return m_pos; };
    bool operator==(const EventPosition& other);
private:
    bool m_valid;
    thread::security::SecurityList<Event*>::ListNodePos m_pos;
};

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
    virtual void SetPos(EventPosition pos) = 0;
    virtual EventPosition GetPos() = 0;
    virtual GHandle GetHandle() = 0;
    virtual bool IsOnHeap() = 0;
    virtual void Free(EventEngine* engine) = 0;
};

class CallbackEvent: public Event
{
public:
    CallbackEvent(GHandle handle, EventType type, bool is_heap, std::function<void(Event*, EventEngine*)> callback);
    virtual void HandleEvent(EventEngine* engine) override;
    inline virtual std::string Name() override { return "CallbackEvent"; }
    inline virtual void SetEventType(EventType type) override { m_type = type; }
    inline virtual EventType GetEventType() override { return m_type; }
    inline virtual void SetPos(EventPosition pos) override { m_pos = pos; }
    inline virtual EventPosition GetPos() override { return m_pos; }
    inline virtual GHandle GetHandle() override { return m_handle; }
    inline virtual bool IsOnHeap() override { return m_is_heap; }
    virtual void Free(EventEngine* engine) override;
private:
    EventPosition m_pos;
    bool m_is_heap;
    EventType m_type;
    GHandle m_handle;
    std::function<void(Event*, EventEngine*)> m_callback;
};

class CoroutineEvent: public Event
{
public:
    CoroutineEvent(GHandle handle, EventEngine* engine, EventType type, bool is_heap);
    virtual void HandleEvent(EventEngine* engine) override;
    inline virtual std::string Name() override { return "CoroutineEvent"; }
    inline virtual void SetEventType(EventType type) override { m_type = type; }
    inline virtual EventType GetEventType() override { return m_type; }
    inline virtual void SetPos(EventPosition pos) override { m_pos = pos; }
    inline virtual EventPosition GetPos() override { return m_pos; }
    inline virtual GHandle GetHandle() override { return m_handle; }
    inline virtual bool IsOnHeap() override { return m_is_heap; }
    
    virtual void Free(EventEngine* engine) override;
    
    void AddCoroutine(coroutine::Coroutine* coroutine);
private:
    std::shared_mutex m_mtx;
    GHandle m_handle;
    bool m_is_heap;
    EventPosition m_pos;
    EventType m_type;
    EventEngine* m_engine;
    std::queue<coroutine::Coroutine*> m_coroutines;
};

class ListenEvent: public Event
{
public:
    ListenEvent(GHandle handle, std::shared_ptr<Strategy> strategy \
        , scheduler::EventScheduler* net_scheduler, scheduler::CoroutineScheduler* co_scheduler, bool is_heap);
    virtual void HandleEvent(EventEngine* engine) override;
    inline virtual std::string Name() override { return "ListenEvent"; }
    inline virtual void SetEventType(EventType type) override {}
    inline virtual EventType GetEventType() override { return kEventTypeRead; }
    inline virtual void SetPos(EventPosition pos) override { m_pos = pos; }
    inline virtual EventPosition GetPos() override { return m_pos; }
    inline virtual GHandle GetHandle() override { return m_handle; }
    inline virtual bool IsOnHeap() override { return m_is_heap; }
    
    virtual void Free(EventEngine* engine) override;
private:
    coroutine::Coroutine CreateTcpSocket(EventEngine* engine);
private:
    GHandle m_handle;
    bool m_is_heap;
    scheduler::EventScheduler* m_net_scheduler;
    scheduler::CoroutineScheduler* m_co_scheduler;
    std::shared_ptr<Strategy> m_strategy;
    EventPosition m_pos;
};

class TimeEvent: public Event
{
public:
    /*
        if timer is cancled and callback is not executed, Success while return false
    */
    class Timer: public std::enable_shared_from_this<Timer> 
    {
    public:
        using ptr = std::shared_ptr<Timer>;
        class TimerCompare
        {
        public:
            bool operator()(const Timer::ptr &a, const Timer::ptr &b);
        };
        Timer(uint64_t during_time, std::function<void(Timer::ptr)> &&func);
        uint64_t GetDuringTime();  
        uint64_t GetExpiredTime();
        uint64_t GetRemainTime();
        inline std::any& GetContext() { return m_context; };
        void SetDuringTime(uint64_t during_time);
        void Execute();
        void Cancle();
        bool Success();
    private:
        std::any m_context;
        uint64_t m_expiredTime;
        uint64_t m_duringTime;
        std::atomic_bool m_cancle;
        std::atomic_bool m_success;
        std::function<void(Timer::ptr)> m_rightHandle;
    };
    TimeEvent(GHandle handle, bool is_heap);
    virtual std::string Name() override { return "TimeEvent"; };
    virtual void HandleEvent(EventEngine* engine) override;
    inline virtual void SetEventType(EventType type) override {};
    inline virtual EventType GetEventType() override { return kEventTypeRead; };
    inline virtual void SetPos(EventPosition pos) override { m_pos = pos; };
    inline virtual EventPosition GetPos() override { return m_pos; };
    inline virtual GHandle GetHandle() override { return m_handle; }
    inline virtual bool IsOnHeap() override { return m_is_heap; }
    virtual void Free(EventEngine* engine) override;
    Timer::ptr AddTimer(uint64_t during_time, std::function<void(Timer::ptr)> &&func); // ms
    void ReAddTimer(uint64_t during_time, Timer::ptr timer);
private:
    void UpdateTimers();
private:
    GHandle m_handle;
    bool m_is_heap;
    std::shared_mutex m_mutex;
    std::priority_queue<Timer::ptr> m_timers;
    EventPosition m_pos;
};

class WaitEvent: public Event
{
public:
    WaitEvent(async::AsyncTcpSocket* socket, EventType type, bool is_heap);
    inline virtual std::string Name() override { return "WaitEvent"; }
    virtual void HandleEvent(EventEngine* engine) override;
    inline virtual void SetEventType(EventType type) override { m_type = type; }
    inline virtual EventType GetEventType() override { return m_type; }
    inline virtual void SetPos(EventPosition pos) override { m_pos = pos; }
    inline virtual EventPosition GetPos() override { return m_pos; }
    inline virtual bool IsOnHeap() override { return m_is_heap; }
    virtual void Free(EventEngine* engine) override;
    virtual GHandle GetHandle() override;
    inline async::AsyncTcpSocket* GetAsyncTcpSocket() { return m_socket; }
    void SetWaitCoroutine(coroutine::Coroutine* co);
    virtual ~WaitEvent() = default;
protected:
    EventType m_type;
    bool m_is_heap;
    async::AsyncTcpSocket* m_socket;
    EventPosition m_pos;
    coroutine::Coroutine* m_waitco;
};

class RecvEvent: public WaitEvent
{
public:
    RecvEvent(async::AsyncTcpSocket* socket, bool is_heap);
    inline virtual std::string Name() override { return "RecvEvent"; }
    virtual void HandleEvent(EventEngine* engine) override;
    virtual ~RecvEvent() = default;
};

class SendEvent: public WaitEvent
{
public:
    SendEvent(async::AsyncTcpSocket* socket, bool is_heap);
    inline virtual std::string Name() override { return "SendEvent"; }
    virtual void HandleEvent(EventEngine* engine) override;
    virtual ~SendEvent() = default;
};
    
}
#endif