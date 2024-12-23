#ifndef GALAY_EVENT_HPP
#define GALAY_EVENT_HPP

#include "galay/common/Base.h"
#include "concurrentqueue/moodycamel/concurrentqueue.h"
#include "Time.h"
#include "EventEngine.h"
#include "Internal.h"
#include "Session.hpp"
#include <queue>
#include <shared_mutex>

namespace galay
{
    template <typename Socket>
    class CallbackStore;
}

namespace galay::details
{
#define DEFAULT_READ_BUFFER_SIZE    2048
#define DEFAULT_IO_EVENTS_SIZE      1024

extern std::string ToString(EventType type);

class EventEngine;
class IOEventAction;
class CoroutineScheduler;

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

class TimeEvent final: public Event,  public std::enable_shared_from_this<TimeEvent>
{
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    static TimeEventIDStore g_idStore; 
#endif
public:
    using ptr = std::shared_ptr<TimeEvent>;
    using wptr = std::weak_ptr<TimeEvent>;
    static bool CreateHandle(GHandle& handle);
    
    TimeEvent(GHandle handle, EventEngine* engine);
    std::string Name() override { return "TimeEvent"; };
    void HandleEvent(EventEngine* engine) override;
    EventType GetEventType() override { return kEventTypeTimer; };
    GHandle GetHandle() override { return m_handle; }
    bool SetEventEngine(EventEngine* engine) override;
    EventEngine* BelongEngine() override;
    Timer::ptr AddTimer(uint64_t during_time, std::function<void(std::weak_ptr<details::TimeEvent>, Timer::ptr)> &&func); // ms
    void AddTimer(const uint64_t timeout, const Timer::ptr& timer);
    ~TimeEvent() override;
private:
#ifdef __linux__
    void UpdateTimers();
#endif
private:
    GHandle m_handle;
    std::atomic<EventEngine*> m_engine;
    std::shared_mutex m_mutex;
    std::priority_queue<Timer::ptr, std::vector<std::shared_ptr<galay::Timer>>, Timer::TimerCompare> m_timers;
};
//#if defined(USE_EPOLL) && !defined(USE_AIO)


template <typename SocketType>
class ListenEvent final : public Event
{
public:
    ListenEvent(EventEngine* engine, SocketType* socket, CallbackStore<SocketType>* store);
    void HandleEvent(EventEngine* engine) override;
    std::string Name() override;
    EventType GetEventType() override;
    GHandle GetHandle() override;
    bool SetEventEngine(EventEngine* engine) override;
    EventEngine* BelongEngine() override;
    ~ListenEvent() override;
private:
    Coroutine CreateConnection(EventEngine* engine);
private:
    std::atomic<EventEngine*> m_engine;
    SocketType* m_socket;
    CallbackStore<SocketType>* m_store;
};


class WaitEvent: public Event
{
public:
    using Coroutine_wptr = std::weak_ptr<Coroutine>;
    WaitEvent();
    /*
        OnWaitPrepare() return false coroutine will not suspend, else suspend
    */
    virtual bool OnWaitPrepare(Coroutine_wptr co, void* ctx) = 0;
    bool SetEventEngine(EventEngine* engine) override;
    EventEngine* BelongEngine() override;
    ~WaitEvent() override = default;
protected:
    std::atomic<EventEngine*> m_engine;
    Coroutine_wptr m_waitco;
};

enum NetWaitEventType
{
    kWaitEventTypeError,
    kTcpWaitEventTypeAccept,
    kTcpWaitEventTypeSslAccept,
    kTcpWaitEventTypeRecv,
    kTcpWaitEventTypeSslRecv,
    kTcpWaitEventTypeSend,
    kTcpWaitEventTypeSslSend,
    kTcpWaitEventTypeConnect,
    kTcpWaitEventTypeSslConnect,
    kWaitEventTypeClose,
    kWaitEventTypeSslClose,
    kUdpWaitEventTypeRecvFrom,
    kUdpWaitEventTypeSendTo,
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
    explicit NetWaitEvent(AsyncNetIo* socket);
    
    std::string Name() override;
    bool OnWaitPrepare(Coroutine_wptr co, void* ctx) override;
    void HandleEvent(EventEngine* engine) override;
    EventType GetEventType() override;
    GHandle GetHandle() override;
    void ResetNetWaitEventType(const NetWaitEventType type) { m_type = type; }
    AsyncNetIo* GetAsyncTcpSocket() const { return m_socket; }
    ~NetWaitEvent() override;
protected:
    bool OnTcpAcceptWaitPrepare(const Coroutine_wptr co, void* ctx);
    bool OnTcpRecvWaitPrepare(const Coroutine_wptr co, void* ctx);
    bool OnTcpSendWaitPrepare(const Coroutine_wptr co, void* ctx);
    bool OnTcpConnectWaitPrepare(const Coroutine_wptr co, void* ctx);
    bool OnCloseWaitPrepare(const Coroutine_wptr co, void* ctx);
    bool OnUdpRecvfromWaitPrepare(const Coroutine_wptr co, void* ctx);
    bool OnUdpSendtoWaitPrepare(const Coroutine_wptr co, void* ctx);


    void HandleErrorEvent(EventEngine* engine);
    void HandleTcpAcceptEvent(EventEngine* engine);
    void HandleTcpRecvEvent(EventEngine* engine);
    void HandleTcpSendEvent(EventEngine* engine);
    void HandleTcpConnectEvent(EventEngine* engine);
    void HandleCloseEvent(EventEngine* engine);
    void HandleUdpRecvfromEvent(EventEngine* engine);
    void HandleUdpSendtoEvent(EventEngine* engine);

    // return recvByte
    virtual int TcpDealRecv(TcpIOVec* iov);
    // return sendByte
    virtual int TcpDealSend(TcpIOVec* iov);

    virtual int UdpDealRecvfrom(UdpIOVec *iov);
    virtual int UdpDealSendto(UdpIOVec* iov);
protected:
    NetWaitEventType m_type;
    void *m_ctx{};
    AsyncNetIo* m_socket;
};

class NetSslWaitEvent final : public NetWaitEvent
{
public:
    explicit NetSslWaitEvent(AsyncSslNetIo* socket);
    std::string Name() override;
    EventType GetEventType() override;
    bool OnWaitPrepare(Coroutine_wptr co, void* ctx) override;
    void HandleEvent(EventEngine* engine) override;
    AsyncSslNetIo* GetAsyncTcpSocket() const;
    ~NetSslWaitEvent() override;
protected:
    bool OnTcpSslAcceptWaitPrepare(const Coroutine_wptr co, void* ctx);
    bool OnTcpSslConnectWaitPrepare(const Coroutine_wptr co, void* ctx);
    bool OnTcpSslRecvWaitPrepare(const Coroutine_wptr co, void* ctx);
    bool OnTcpSslSendWaitPrepare(const Coroutine_wptr co, void* ctx);
    bool OnTcpSslCloseWaitPrepare(const Coroutine_wptr co, void* ctx);

    void HandleTcpSslAcceptEvent(EventEngine* engine);
    void HandleTcpSslConnectEvent(EventEngine* engine);
    void HandleTcpSslRecvEvent(EventEngine* engine);
    void HandleTcpSslSendEvent(EventEngine* engine);
    void HandleTcpSslCloseEvent(EventEngine* engine);

    EventType CovertSSLErrorToEventType() const;
    // return recvByte
    int TcpDealRecv(TcpIOVec* iovc) override;
    // return sendByte
    int TcpDealSend(TcpIOVec* iovc) override;
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
    kFileIoWaitEventTypeClose
};

class FileIoWaitEvent final : public WaitEvent
{
public:
    explicit FileIoWaitEvent(AsyncFileIo* fileio);
    std::string Name() override;
    bool OnWaitPrepare(Coroutine_wptr co, void* ctx) override;
    void HandleEvent(EventEngine* engine) override;
    EventType GetEventType() override;
    GHandle GetHandle() override;
    void ResetFileIoWaitEventType(FileIoWaitEventType type) { m_type = type; }
    AsyncFileIo* GetAsyncTcpSocket() const { return m_fileio; }
    ~FileIoWaitEvent() override;
private:
#ifdef __linux__
    bool OnAioWaitPrepare(Coroutine_wptr co, void* ctx);
    void HandleAioEvent(EventEngine* engine);
#endif
    bool OnKReadWaitPrepare(Coroutine_wptr co, void* ctx);
    void HandleKReadEvent(EventEngine* engine);
    bool OnKWriteWaitPrepare(Coroutine_wptr co, void* ctx);
    void HandleKWriteEvent(EventEngine* engine);

    bool OnCloseWaitPrepare(const Coroutine_wptr co, void* ctx);
    
private:
    void *m_ctx{};
    AsyncFileIo* m_fileio;
    FileIoWaitEventType m_type;
};

//#elif defined(USE_KQUEUE)




//#endif
    
}

#include "Event.tcc"

#endif