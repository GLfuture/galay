#ifndef __GALAY_EVENT_H__
#define __GALAY_EVENT_H__

#include "galay/common/Base.h"
#include "concurrentqueue/moodycamel/concurrentqueue.h"
#include <any>
#include <queue>
#include <mutex>
#include <atomic>
#include <memory>
#include <functional>
#include <shared_mutex>
#include <openssl/ssl.h>

namespace galay::coroutine
{
    class Coroutine;
}

namespace galay::async
{
    class AsyncTcpSocket;
    class AsyncTcpSslSocket;
    class IOVec;
}

namespace galay::action
{
    class TcpEventAction;
    class TcpSslEventAction;
}

namespace galay
{
    class TcpCallbackStore;
    class TcpSslCallbackStore;
}

namespace galay::event
{
#define DEFAULT_READ_BUFFER_SIZE    2048

enum EventType
{
    kEventTypeNone,
    kEventTypeError,
    kEventTypeRead,
    kEventTypeWrite,
    kEventTypeTimer,
};

extern std::string ToString(EventType type);

class EventEngine;
class Event;

//must be alloc at heap
class Event
{
public:
    virtual std::string Name() = 0;
    virtual void HandleEvent(EventEngine* engine) = 0;
    virtual EventType GetEventType() = 0;
    virtual GHandle GetHandle() = 0;
    virtual EventEngine*& BelongEngine() = 0;
};

class CallbackEvent: public Event
{
public:
    CallbackEvent(GHandle handle, EventType type, std::function<void(Event*, EventEngine*)> callback);
    virtual void HandleEvent(EventEngine* engine) override;
    inline virtual std::string Name() override { return "CallbackEvent"; }
    inline virtual EventType GetEventType() override { return m_type; }
    inline virtual GHandle GetHandle() override { return m_handle; }
    virtual EventEngine*& BelongEngine() override;
    virtual ~CallbackEvent();
private:
    EventType m_type;
    GHandle m_handle;
    EventEngine* m_engine;
    std::function<void(Event*, EventEngine*)> m_callback;
};
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

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)

#define DEFAULT_TIMEEVENT_ID_CAPACITY 1024

class TimeEventIDStore
{
public:
    //[0, capacity)
    TimeEventIDStore(int capacity);
    bool GetEventId(int& id);
    bool RecycleEventId(int id);
    ~TimeEventIDStore();
private:
    int *m_temp;
    int m_capacity;
    moodycamel::ConcurrentQueue<int> m_eventIds;
};
#endif

class TimeEvent: public Event
{
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    static TimeEventIDStore IDStroe; 
#endif
public:
    using ptr = std::shared_ptr<TimeEvent>;
    static bool CreateHandle(GHandle& handle);
    
    TimeEvent(GHandle handle, EventEngine* engine);
    virtual std::string Name() override { return "TimeEvent"; };
    virtual void HandleEvent(EventEngine* engine) override;
    inline virtual EventType GetEventType() override { return kEventTypeTimer; };
    inline virtual GHandle GetHandle() override { return m_handle; }
    virtual EventEngine* &BelongEngine() override;
    Timer::ptr AddTimer(int64_t during_time, std::function<void(Timer::ptr)> &&func); // ms
    void ReAddTimer(int64_t during_time, Timer::ptr timer);
    virtual ~TimeEvent();
private:
#ifdef __linux__
    void UpdateTimers();
#endif
private:
    GHandle m_handle;
    EventEngine* m_engine;
    std::shared_mutex m_mutex;
    std::priority_queue<Timer::ptr> m_timers;
};
//#if defined(USE_EPOLL) && !defined(USE_AIO)

class TcpWaitEvent;
class TcpSslWaitEvent;

class ListenEvent: public Event
{
public:
    ListenEvent(EventEngine* engine, async::AsyncTcpSocket* socket, TcpCallbackStore* callback_store);
    virtual void HandleEvent(EventEngine* engine) override;
    inline virtual std::string Name() override { return "ListenEvent"; }
    inline virtual EventType GetEventType() override { return kEventTypeRead; }
    virtual GHandle GetHandle() override;
    virtual EventEngine*& BelongEngine() override;
    virtual ~ListenEvent();
private:
    coroutine::Coroutine CreateTcpSocket(EventEngine* engine);
private:
    EventEngine* m_engine;
    async::AsyncTcpSocket* m_socket;
    TcpCallbackStore* m_callback_store;
};

class SslListenEvent: public Event
{
public:
    SslListenEvent(EventEngine* engine, async::AsyncTcpSslSocket* socket, TcpSslCallbackStore* callback_store);
    virtual void HandleEvent(EventEngine* engine) override;
    inline virtual std::string Name() override { return "SslListenEvent"; }
    inline virtual EventType GetEventType() override { return kEventTypeRead; }
    virtual GHandle GetHandle() override;
    virtual EventEngine*& BelongEngine() override;
    virtual ~SslListenEvent();
private:
    coroutine::Coroutine CreateTcpSslSocket(EventEngine* engine);
private:
    EventEngine* m_engine;
    async::AsyncTcpSslSocket* m_socket;
    TcpSslCallbackStore* m_callback_store;
    action::TcpSslEventAction* m_action;
};


class WaitEvent: public Event
{
public:
    WaitEvent(EventEngine* engine = nullptr);
    /*
        OnWaitPrepare() return false coroutine will not suspend, else suspend
    */
    virtual bool OnWaitPrepare(coroutine::Coroutine* co, void* ctx) = 0;
    virtual EventEngine*& BelongEngine() override;
    virtual ~WaitEvent() = default;
protected:
    event::EventEngine* m_engine;
    coroutine::Coroutine* m_waitco;
};

enum TcpWaitEventType
{
    kWaitEventTypeError,
    kWaitEventTypeAccept,
    kWaitEventTypeSslAccept,
    kWaitEventTypeRecv,
    kWaitEventTypeSslRecv,
    kWaitEventTypeSend,
    kWaitEventTypeSslSend,
    kWaitEventTypeConnect,
    kWaitEventTypeSslConnect,
    kWaitEventTypeClose,
    kWaitEventTypeSslClose,
};

class TcpWaitEvent: public WaitEvent
{
public:
    TcpWaitEvent(async::AsyncTcpSocket* socket);
    
    virtual std::string Name() override;
    virtual bool OnWaitPrepare(coroutine::Coroutine* co, void* ctx) override;
    virtual void HandleEvent(EventEngine* engine) override;
    virtual EventType GetEventType() override;
    virtual GHandle GetHandle() override;
    inline void ResetNetWaitEventType(TcpWaitEventType type) { m_type = type; }
    inline async::AsyncTcpSocket* GetAsyncTcpSocket() { return m_socket; }
    virtual ~TcpWaitEvent();
protected:
    bool OnAcceptWaitPrepare(coroutine::Coroutine* co, void* ctx);
    bool OnRecvWaitPrepare(coroutine::Coroutine* co, void* ctx);
    bool OnSendWaitPrepare(coroutine::Coroutine* co, void* ctx);
    bool OnConnectWaitPrepare(coroutine::Coroutine* co, void* ctx);
    bool OnCloseWaitPrepare(coroutine::Coroutine* co, void* ctx);

    void HandleErrorEvent(EventEngine* engine);
    void HandleAcceptEvent(EventEngine* engine);
    void HandleRecvEvent(EventEngine* engine);
    void HandleSendEvent(EventEngine* engine);
    void HandleConnectEvent(EventEngine* engine);
    void HandleCloseEvent(EventEngine* engine);

    // return recvByte
    int DealRecv(async::IOVec* iov);
    // return sendByte
    int DealSend(async::IOVec* iov);
protected:
    TcpWaitEventType m_type;
    void *m_ctx;
    async::AsyncTcpSocket* m_socket;
};

class TcpSslWaitEvent: public TcpWaitEvent
{
public:
    TcpSslWaitEvent(async::AsyncTcpSslSocket* socket);
    virtual std::string Name() override;
    virtual EventType GetEventType() override;
    virtual bool OnWaitPrepare(coroutine::Coroutine* co, void* ctx) override;
    virtual void HandleEvent(EventEngine* engine) override;
    async::AsyncTcpSslSocket* GetAsyncTcpSocket();
    virtual ~TcpSslWaitEvent();
protected:
    bool OnSslAcceptWaitPrepare(coroutine::Coroutine* co, void* ctx);
    bool OnSslConnectWaitPrepare(coroutine::Coroutine* co, void* ctx);
    bool OnSslRecvWaitPrepare(coroutine::Coroutine* co, void* ctx);
    bool OnSslSendWaitPrepare(coroutine::Coroutine* co, void* ctx);
    bool OnSslCloseWaitPrepare(coroutine::Coroutine* co, void* ctx);

    void HandleSslAcceptEvent(EventEngine* engine);
    void HandleSslConnectEvent(EventEngine* engine);
    void HandleSslRecvEvent(EventEngine* engine);
    void HandleSslSendEvent(EventEngine* engine);
    void HandleSslCloseEvent(EventEngine* engine);

    EventType CovertSSLErrorToEventType();
    // return recvByte
    int DealRecv(async::IOVec* iovc);
    // return sendByte
    int DealSend(async::IOVec* iovc);
private:
    int m_ssl_error;
};

class UdpWaitEvent: public WaitEvent
{
public:
    
private:
    sockaddr* m_ToAddr;
    sockaddr* m_FromAddr;
};

//#elif defined(USE_KQUEUE)




//#endif
    
}
#endif