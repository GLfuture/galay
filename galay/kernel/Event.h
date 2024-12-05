#ifndef GALAY_EVENT_H
#define GALAY_EVENT_H

#include "galay/common/Base.h"
#include "concurrentqueue/moodycamel/concurrentqueue.h"
#include <any>
#include <queue>
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
    class AsyncNetIo;
    class AsyncSslNetIo;
    class AsyncFileIo; 
}

namespace galay::action
{
    class IOEventAction;
}

namespace galay
{
    class TcpCallbackStore;
    class TcpSslCallbackStore;
    class IOVec;
}

namespace galay::event
{
#define DEFAULT_READ_BUFFER_SIZE    2048
#define DEFAULT_IO_EVENTS_SIZE      1024

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
    virtual bool SetEventEngine(EventEngine* engine) = 0;
    virtual EventEngine* BelongEngine() = 0;
    virtual ~Event() = default;
};

class CallbackEvent final : public Event
{
public:
    CallbackEvent(GHandle handle, EventType type, std::function<void(Event*, EventEngine*)> callback);
    void HandleEvent(EventEngine* engine) override;
    std::string Name() override { return "CallbackEvent"; }
    EventType GetEventType() override { return m_type; }
    GHandle GetHandle() override { return m_handle; }
    bool SetEventEngine(EventEngine* engine) override;
    EventEngine* BelongEngine() override;
    ~CallbackEvent() override;
private:
    EventType m_type;
    GHandle m_handle;
    std::atomic<EventEngine*> m_engine;
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
        bool operator()(const Timer::ptr &a, const Timer::ptr &b) const;
    };
    Timer(int64_t during_time, std::function<void(Timer::ptr)> &&func);
    int64_t GetDuringTime() const;
    int64_t GetExpiredTime() const;
    int64_t GetRemainTime() const;
    std::any& GetContext() { return m_context; };
    void SetDuringTime(int64_t during_time);
    void Execute();
    void Cancle();
    bool Success() const;
private:
    std::any m_context;
    int64_t m_expiredTime{};
    int64_t m_duringTime{};
    std::atomic_bool m_cancle;
    std::atomic_bool m_success;
    std::function<void(Timer::ptr)> m_rightHandle;
};

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)

#define DEFAULT_TIMEEVENT_ID_CAPACITY 1024

class TimeEventIDStore
{
public:
    using ptr = std::shared_ptr<TimeEventIDStore>;
    //[0, capacity)
    explicit TimeEventIDStore(int capacity);
    bool GetEventId(int& id);
    bool RecycleEventId(int id);
private:
    int *m_temp;
    int m_capacity;
    moodycamel::ConcurrentQueue<int> m_eventIds;
};
#endif

class TimeEvent final : public Event
{
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    static TimeEventIDStore g_idStore; 
#endif
public:
    using ptr = std::shared_ptr<TimeEvent>;
    static bool CreateHandle(GHandle& handle);
    
    TimeEvent(GHandle handle, EventEngine* engine);
    std::string Name() override { return "TimeEvent"; };
    void HandleEvent(EventEngine* engine) override;
    EventType GetEventType() override { return kEventTypeTimer; };
    GHandle GetHandle() override { return m_handle; }
    bool SetEventEngine(EventEngine* engine) override;
    EventEngine* BelongEngine() override;
    Timer::ptr AddTimer(int64_t during_time, std::function<void(Timer::ptr)> &&func); // ms
    void ReAddTimer(int64_t during_time, const Timer::ptr& timer);
    ~TimeEvent() override;
private:
#ifdef __linux__
    void UpdateTimers();
#endif
private:
    GHandle m_handle;
    std::atomic<EventEngine*> m_engine;
    std::shared_mutex m_mutex;
    std::priority_queue<Timer::ptr> m_timers;
};
//#if defined(USE_EPOLL) && !defined(USE_AIO)


class ListenEvent final : public Event
{
public:
    ListenEvent(EventEngine* engine, async::AsyncNetIo* socket, TcpCallbackStore* callback_store);
    void HandleEvent(EventEngine* engine) override;
    std::string Name() override { return "ListenEvent"; }
    EventType GetEventType() override { return kEventTypeRead; }
    GHandle GetHandle() override;
    bool SetEventEngine(EventEngine* engine) override;
    EventEngine* BelongEngine() override;
    ~ListenEvent() override;
private:
    coroutine::Coroutine CreateTcpSocket(EventEngine* engine);
private:
    std::atomic<EventEngine*> m_engine;
    async::AsyncNetIo* m_socket;
    TcpCallbackStore* m_callback_store;
};

class SslListenEvent final : public Event
{
public:
    SslListenEvent(EventEngine* engine, async::AsyncSslNetIo* socket, TcpSslCallbackStore* callback_store);
    void HandleEvent(EventEngine* engine) override;
    std::string Name() override { return "SslListenEvent"; }
    EventType GetEventType() override { return kEventTypeRead; }
    GHandle GetHandle() override;
    bool SetEventEngine(EventEngine* engine) override;
    EventEngine* BelongEngine() override;
    ~SslListenEvent() override;
private:
    coroutine::Coroutine CreateTcpSslSocket(EventEngine* engine);
private:
    std::atomic<EventEngine*> m_engine;
    async::AsyncSslNetIo* m_socket;
    TcpSslCallbackStore* m_callback_store;
    action::IOEventAction* m_action;
};


class WaitEvent: public Event
{
public:
    WaitEvent();
    /*
        OnWaitPrepare() return false coroutine will not suspend, else suspend
    */
    virtual bool OnWaitPrepare(coroutine::Coroutine* co, void* ctx) = 0;
    bool SetEventEngine(EventEngine* engine) override;
    EventEngine* BelongEngine() override;
    ~WaitEvent() override = default;
protected:
    std::atomic<EventEngine*> m_engine;
    coroutine::Coroutine* m_waitco;
};

enum NetWaitEventType
{
    kTcpWaitEventTypeError,
    kTcpWaitEventTypeAccept,
    kTcpWaitEventTypeSslAccept,
    kTcpWaitEventTypeRecv,
    kTcpWaitEventTypeSslRecv,
    kTcpWaitEventTypeSend,
    kTcpWaitEventTypeSslSend,
    kTcpWaitEventTypeConnect,
    kTcpWaitEventTypeSslConnect,
    kTcpWaitEventTypeClose,
    kTcpWaitEventTypeSslClose,
};

enum CommonFailedType
{
    eCommonOtherFailed = -2,
    eCommonNonBlocking = -1,
    eCommonDisConnect = 0,
};

class NetWaitEvent: public WaitEvent
{
public:
    explicit NetWaitEvent(async::AsyncNetIo* socket);
    
    std::string Name() override;
    bool OnWaitPrepare(coroutine::Coroutine* co, void* ctx) override;
    void HandleEvent(EventEngine* engine) override;
    EventType GetEventType() override;
    GHandle GetHandle() override;
    void ResetNetWaitEventType(const NetWaitEventType type) { m_type = type; }
    async::AsyncNetIo* GetAsyncTcpSocket() const { return m_socket; }
    ~NetWaitEvent() override;
protected:
    bool OnAcceptWaitPrepare(const coroutine::Coroutine* co, void* ctx) const;
    bool OnRecvWaitPrepare(const coroutine::Coroutine* co, void* ctx);
    bool OnSendWaitPrepare(const coroutine::Coroutine* co, void* ctx);
    bool OnConnectWaitPrepare(coroutine::Coroutine* co, void* ctx) const;
    bool OnCloseWaitPrepare(const coroutine::Coroutine* co, void* ctx);

    void HandleErrorEvent(EventEngine* engine);
    void HandleAcceptEvent(EventEngine* engine);
    void HandleRecvEvent(EventEngine* engine);
    void HandleSendEvent(EventEngine* engine);
    void HandleConnectEvent(EventEngine* engine) const;
    void HandleCloseEvent(EventEngine* engine);

    // return recvByte
    virtual int DealRecv(IOVec* iov);
    // return sendByte
    virtual int DealSend(IOVec* iov);
protected:
    NetWaitEventType m_type;
    void *m_ctx{};
    async::AsyncNetIo* m_socket;
};

class NetSslWaitEvent final : public NetWaitEvent
{
public:
    explicit NetSslWaitEvent(async::AsyncSslNetIo* socket);
    std::string Name() override;
    EventType GetEventType() override;
    bool OnWaitPrepare(coroutine::Coroutine* co, void* ctx) override;
    void HandleEvent(EventEngine* engine) override;
    async::AsyncSslNetIo* GetAsyncTcpSocket() const;
    ~NetSslWaitEvent() override;
protected:
    bool OnSslAcceptWaitPrepare(const coroutine::Coroutine* co, void* ctx);
    bool OnSslConnectWaitPrepare(const coroutine::Coroutine* co, void* ctx);
    bool OnSslRecvWaitPrepare(const coroutine::Coroutine* co, void* ctx);
    bool OnSslSendWaitPrepare(const coroutine::Coroutine* co, void* ctx);
    bool OnSslCloseWaitPrepare(const coroutine::Coroutine* co, void* ctx);

    void HandleSslAcceptEvent(EventEngine* engine);
    void HandleSslConnectEvent(EventEngine* engine);
    void HandleSslRecvEvent(EventEngine* engine);
    void HandleSslSendEvent(EventEngine* engine);
    void HandleSslCloseEvent(EventEngine* engine);

    EventType CovertSSLErrorToEventType() const;
    // return recvByte
    int DealRecv(IOVec* iovc) override;
    // return sendByte
    int DealSend(IOVec* iovc) override;
private:
    int m_ssl_error{};
};

class UdpWaitEvent: public WaitEvent
{
public:
    
private:
    sockaddr* m_ToAddr = nullptr;
    sockaddr* m_FromAddr = nullptr;
};

enum FileIoWaitEventType
{
    kFileIoWaitEventTypeError,
#ifdef __linux__
    kFileIoWaitEventTypeLinuxAio,
#endif
    kFileIoWaitEventTypeRead,
    kFileIoWaitEventTypeWrite,
};

class FileIoWaitEvent final : public WaitEvent
{
public:
    explicit FileIoWaitEvent(async::AsyncFileIo* fileio);
    std::string Name() override;
    bool OnWaitPrepare(coroutine::Coroutine* co, void* ctx) override;
    void HandleEvent(EventEngine* engine) override;
    EventType GetEventType() override;
    GHandle GetHandle() override;
    void ResetFileIoWaitEventType(FileIoWaitEventType type) { m_type = type; }
    async::AsyncFileIo* GetAsyncTcpSocket() const { return m_fileio; }
    ~FileIoWaitEvent() override;
private:
#ifdef __linux__
    bool OnAioWaitPrepare(coroutine::Coroutine* co, void* ctx);
    void HandleAioEvent(EventEngine* engine);
#endif
    bool OnKReadWaitPrepare(coroutine::Coroutine* co, void* ctx);
    void HandleKReadEvent(EventEngine* engine);
    bool OnKWriteWaitPrepare(coroutine::Coroutine* co, void* ctx);
    void HandleKWriteEvent(EventEngine* engine);
private:
    void *m_ctx{};
    async::AsyncFileIo* m_fileio;
    FileIoWaitEventType m_type;
};

//#elif defined(USE_KQUEUE)




//#endif
    
}
#endif