#ifndef GALAY_SERVER_HPP
#define GALAY_SERVER_HPP

#include <string>
#include <vector>
#include <atomic>
#include <string_view>
#include <unordered_map>
#include <concurrentqueue/moodycamel/concurrentqueue.h>
#include "Async.hpp"
#include "Session.hpp"

namespace galay::details
{

Coroutine<void> CreateConnection(RoutineCtx ctx, galay::AsyncTcpSocket* socket, EventScheduler *engine);
Coroutine<void> CreateConnection(RoutineCtx ctx, AsyncTcpSslSocket* socket, EventScheduler *engine);


template <typename Socket>
class CallbackStore
{
public:
    using callback_t = std::function<Coroutine<void>(RoutineCtx,typename Connection<Socket>::ptr)>;
    static void RegisteCallback(callback_t callback);
    static void CreateConnAndExecCallback(EventScheduler* scheduler, Socket* socket);
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
    SocketType* m_socket;
    std::atomic<EventScheduler*> m_scheduler;
};



};


namespace galay 
{

#define DEFAULT_SERVER_BACKLOG                          32

#define DEFAULT_HTTP_MAX_HEADER_SIZE                    4096

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

struct HttpServerConfig final: public TcpServerConfig
{
    using ptr = std::shared_ptr<HttpServerConfig>;
    static HttpServerConfig::ptr Create();
    ~HttpServerConfig() override = default;

    uint32_t m_max_header_size = DEFAULT_HTTP_MAX_HEADER_SIZE;

};

template<typename SocketType>
class TcpServer
{
public:
    using callback_t = std::function<Coroutine<void>(RoutineCtx,typename Connection<SocketType>::ptr)>;
    explicit TcpServer(TcpServerConfig::ptr config) :m_config(config) {}
    void Prepare(callback_t callback);
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