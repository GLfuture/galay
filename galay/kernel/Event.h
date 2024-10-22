#ifndef __GALAY_EVENT_H__
#define __GALAY_EVENT_H__

#include "../common/Base.h"
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

namespace galay::action
{
    class NetIoEventAction;
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
    kEventTypeNone = 0,
    kEventTypeRead = 1,
    kEventTypeWrite = 2,
};

class EventEngine;
class Event;

//must be alloc at heap
class Event
{
public:
    virtual std::string Name() = 0;
    virtual void HandleEvent(EventEngine* engine) = 0;
    virtual int GetEventType() = 0;
    virtual GHandle GetHandle() = 0;
    virtual void Free(EventEngine* engine) = 0;
    virtual void SetEventInEngine(bool flag) = 0;
    virtual bool EventInEngine() = 0;
};

class CallbackEvent: public Event
{
public:
    CallbackEvent(GHandle handle, EventType type, std::function<void(Event*, EventEngine*)> callback);
    virtual void HandleEvent(EventEngine* engine) override;
    inline virtual std::string Name() override { return "CallbackEvent"; }
    inline virtual int GetEventType() override { return m_type; }
    inline virtual GHandle GetHandle() override { return m_handle; }
    inline virtual void SetEventInEngine(bool flag) override { m_event_in_engine = flag; }
    inline virtual bool EventInEngine() override { return m_event_in_engine; }
    virtual void Free(EventEngine* engine) override;
private:
    EventType m_type;
    GHandle m_handle;
    bool m_event_in_engine;
    std::function<void(Event*, EventEngine*)> m_callback;
};

class CoroutineEvent: public Event
{
public:
    CoroutineEvent(GHandle handle, EventEngine* engine, EventType type);
    virtual void HandleEvent(EventEngine* engine) override;
    inline virtual std::string Name() override { return "CoroutineEvent"; }
    inline virtual int GetEventType() override { return m_type; }
    inline virtual GHandle GetHandle() override { return m_handle; }
    inline virtual void SetEventInEngine(bool flag) override { m_event_in_engine = flag; }
    inline virtual bool EventInEngine() override { return m_event_in_engine; }
    virtual void Free(EventEngine* engine) override;
    void ResumeCoroutine(coroutine::Coroutine* coroutine);
    
private:
    std::shared_mutex m_mtx;
    GHandle m_handle;
    EventType m_type;
    EventEngine* m_engine;
    bool m_event_in_engine;
    std::queue<coroutine::Coroutine*> m_coroutines;
    
};

class NetWaitEvent;

class ListenEvent: public Event
{
public:
    ListenEvent(async::AsyncTcpSocket* socket, CallbackStore* callback_store \
        , scheduler::EventScheduler* net_scheduler, scheduler::CoroutineScheduler* co_scheduler);
    virtual void HandleEvent(EventEngine* engine) override;
    inline virtual std::string Name() override { return "ListenEvent"; }
    inline virtual int GetEventType() override { return kEventTypeRead; }
    virtual GHandle GetHandle() override;
    inline virtual void SetEventInEngine(bool flag) override { m_event_in_engine = flag; }
    inline virtual bool EventInEngine() override { return m_event_in_engine; }
    virtual void Free(EventEngine* engine) override;
    virtual ~ListenEvent();
private:
    coroutine::Coroutine CreateTcpSocket(EventEngine* engine);
private:
    async::AsyncTcpSocket* m_socket;
    bool m_event_in_engine;
    scheduler::EventScheduler* m_net_scheduler;
    scheduler::CoroutineScheduler* m_co_scheduler;
    CallbackStore* m_callback_store;
    NetWaitEvent* m_net_event;
    action::NetIoEventAction* m_action;
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
    inline virtual int GetEventType() override { return kEventTypeRead; };
    inline virtual GHandle GetHandle() override { return m_handle; }
    inline virtual void SetEventInEngine(bool flag) override { m_event_in_engine = flag; }
    inline virtual bool EventInEngine() override { return m_event_in_engine; }
    virtual void Free(EventEngine* engine) override;
    Timer::ptr AddTimer(int64_t during_time, std::function<void(Timer::ptr)> &&func); // ms
    void ReAddTimer(int64_t during_time, Timer::ptr timer);
private:
    void UpdateTimers();
private:
    GHandle m_handle;
    bool m_event_in_engine;
    std::shared_mutex m_mutex;
    std::priority_queue<Timer::ptr> m_timers;
};

class WaitEvent: public Event
{
public:
    WaitEvent(event::EventEngine* engine);
    virtual std::string Name() = 0;
    /*
        OnWaitPrepare() return false coroutine will not suspend, else suspend
    */
    virtual bool OnWaitPrepare(coroutine::Coroutine* co) = 0;
    virtual void HandleEvent(EventEngine* engine) = 0;
    virtual int GetEventType() = 0;
    
    virtual void Free(EventEngine* engine) = 0;
    virtual GHandle GetHandle() = 0;
    inline event::EventEngine* GetEventEngine() { return m_engine; }
    inline virtual void SetEventInEngine(bool flag) override { m_event_in_engine = flag; }
    inline virtual bool EventInEngine() override { return m_event_in_engine; }
    virtual ~WaitEvent() = default;
protected:
    bool m_event_in_engine;
    event::EventEngine* m_engine;
    coroutine::Coroutine* m_waitco;
};

class NetWaitEvent: public WaitEvent
{
public:
    enum NetWaitEventType
    {
        kWaitEventTypeSocket,
        kWaitEventTypeAccept,
        kWaitEventTypeRecv,
        kWaitEventTypeSend,
        kWaitEventTypeConnect,
        kWaitEventTypeClose,
    };
    NetWaitEvent(event::EventEngine* engine, async::AsyncTcpSocket* socket);
    
    virtual std::string Name() override;
    virtual bool OnWaitPrepare(coroutine::Coroutine* co) override;
    virtual void HandleEvent(EventEngine* engine) override;
    virtual int GetEventType() override;
    virtual GHandle GetHandle() override;
    inline void ResetNetWaitEventType(NetWaitEventType type) { m_type = type; }
    
    inline async::AsyncTcpSocket* GetAsyncTcpSocket() { return m_socket; }
    virtual void Free(EventEngine* engine) override;
private:
    bool OnSocketPrepare(coroutine::Coroutine* co);
    bool OnAcceptPrepare(coroutine::Coroutine* co);
    bool OnRecvWaitPrepare(coroutine::Coroutine* co);
    bool OnSendWaitPrepare(coroutine::Coroutine* co);
    bool OnConnectWaitPrepare(coroutine::Coroutine* co);
    bool OnCloseWaitPrepare(coroutine::Coroutine* co);

    void HandleSocketEvent(EventEngine* engine);
    void HandleAcceptEvent(EventEngine* engine);
    void HandleRecvEvent(EventEngine* engine);
    void HandleSendEvent(EventEngine* engine);
    void HandleConnectEvent(EventEngine* engine);
    void HandleCloseEvent(EventEngine* engine);

    // return recvByte
    int DealRecv();
    // return sendByte
    int DealSend();
private:
    NetWaitEventType m_type;
    async::AsyncTcpSocket* m_socket;
};
    
}
#endif