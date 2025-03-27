#ifndef GALAY_SERVER_HPP
#define GALAY_SERVER_HPP

#include <any>
#include <string>
#include <vector>
#include <atomic>
#include <string_view>
#include <unordered_map>
#include <functional>
#include <concurrentqueue/moodycamel/concurrentqueue.h>
#include "Coroutine.hpp"
#include "Async.hpp"

namespace galay
{

template <typename Socket>
class Connection: public std::enable_shared_from_this<Connection<Socket>>
{
public:
    using uptr = std::unique_ptr<Connection>;
    using EventScheduler = details::EventScheduler;
    using timeout_callback_t = std::function<void(typename Connection<Socket>::uptr)>;

    explicit Connection(EventScheduler* scheduler, std::unique_ptr<Socket> socket);
    
    template <typename CoRtn = void>
    AsyncResult<int, CoRtn> Recv(TcpIOVecHolder& holder, int size, int64_t timeout_ms);

    template <typename CoRtn = void>
    AsyncResult<int, CoRtn> Send(TcpIOVecHolder& holder, int size, int64_t timeout_ms);

    template <typename CoRtn = void>
    AsyncResult<bool, CoRtn> Close();

    EventScheduler *GetScheduler() const;
    std::unique_ptr<Socket>& GetSocket();
private:
    EventScheduler* m_scheduler;
    std::unique_ptr<Socket> m_socket;
};


}

namespace galay::details
{

Coroutine<void> CreateConnection(RoutineCtx ctx, galay::AsyncTcpSocket* socket, EventScheduler *engine);
Coroutine<void> CreateConnection(RoutineCtx ctx, AsyncTcpSslSocket* socket, EventScheduler *engine);


template <typename Socket>
class CallbackStore
{
public:
    using callback_t = std::function<Coroutine<void>(RoutineCtx,typename Connection<Socket>::uptr)>;
    static void RegisteCallback(callback_t callback);
    static void CreateConnAndExecCallback(EventScheduler* scheduler, std::unique_ptr<Socket> socket);
private:
    static callback_t m_callback;
};

template <typename SocketType>
class ListenEvent: public Event
{
public:
    ListenEvent(EventScheduler* scheduler, SocketType* socket);
    void HandleEvent(EventEngine* engine) override;
    std::string Name() override;
    EventType GetEventType() override;
    GHandle GetHandle() override;
    bool SetEventEngine(EventEngine* engine) override;
    EventEngine* BelongEngine() override;
    ~ListenEvent() override;
private:
    void CreateConnection(RoutineCtx ctx, EventScheduler* scheduler);
private:
    std::unique_ptr<SocketType> m_socket;
    std::atomic<EventScheduler*> m_scheduler;
};



};


namespace galay 
{

#define DEFAULT_SERVER_BACKLOG                          32


#define DEFAULT_REQUIRED_EVENT_SCHEDULER_NUM            4

struct TcpServerConfig
{
    using ptr = std::shared_ptr<TcpServerConfig>;
    static TcpServerConfig::ptr Create();
    virtual ~TcpServerConfig() = default;
    /*
        与系统eventScheduler数量有关，取两者较小值
    */
    int m_requiredEventSchedulerNum = DEFAULT_REQUIRED_EVENT_SCHEDULER_NUM;     // 占用前m_requiredEventSchedulerNum个框架eventscheduler
    int m_backlog = DEFAULT_SERVER_BACKLOG;                                     // 半连接队列长度
};

template<typename SocketType>
class TcpServer
{
public:
    using callback_t = std::function<Coroutine<void>(RoutineCtx,typename Connection<SocketType>::uptr)>;
    explicit TcpServer(TcpServerConfig::ptr config) :m_config(config) {}
    void OnCall(callback_t callback);
    //no block
    void Start(THost host);
    void Stop() ;
    TcpServerConfig::ptr GetConfig() { return m_config; }
    inline bool IsRunning() { return m_is_running; }
    ~TcpServer() = default;
protected:
    TcpServerConfig::ptr m_config;
    std::atomic_bool m_is_running;
    std::vector<details::ListenEvent<SocketType>*> m_listen_events;
};


}

#include "Server.tcc"

#endif