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
#include "galay/helper/HttpHelper.h"

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


namespace galay::server 
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


template<typename T>
concept ProtoType = std::default_initializable<T> && requires(T type, const std::string_view& buffer)
{
    { type.DecodePdu(buffer) } -> std::same_as<std::pair<bool, size_t>>;
    { type.EncodePdu() }-> std::same_as<std::string>;
    { type.HasError() } -> std::same_as<bool>;
    { type.GetErrorCode() } -> std::same_as<int>;
    { type.GetErrorString() } -> std::same_as<std::string>;
    { type.Reset() } -> std::same_as<void>;
};

#define DEFAULT_HTTP_KEEPALIVE_TIME_MS              (7500 * 1000)

using namespace galay::http;

template<HttpStatusCode Code>
class CodeResponse
{
public:
    static std::string ResponseStr(HttpVersion version);
    static bool RegisterResponse(HttpResponse response);
private:
    static std::string DefaultResponse(HttpVersion version);
    static std::string DefaultResponseBody() { return ""; }
private:
    static std::string m_responseStr;
};


template<HttpStatusCode Code>
std::string CodeResponse<Code>::m_responseStr = "";

template<typename SocketType>
class HttpRouteHandler 
{
    using SessionPtr = typename galay::Session<SocketType, HttpRequest, HttpResponse>::ptr;
public:
    using HandlerMap = std::unordered_map<HttpMethod, std::unordered_map<std::string, std::function<Coroutine<void>(RoutineCtx,SessionPtr)>>>;
    void AddHandler(HttpMethod method, const std::string& path, std::function<Coroutine<void>(RoutineCtx,SessionPtr)>&& handler);
    
    static HttpRouteHandler<SocketType>* GetInstance();
    Coroutine<std::string> Handler(RoutineCtx ctx, HttpMethod method, const std::string &path, SessionPtr session);
    
private:
    static std::unique_ptr<HttpRouteHandler<SocketType>> m_instance;
    HandlerMap m_handler_map;
};

template<typename SocketType>
std::unique_ptr<HttpRouteHandler<SocketType>> HttpRouteHandler<SocketType>::m_instance = nullptr;


template<typename SocketType>
class HttpServer
{
public:
    using SessionPtr = typename galay::Session<SocketType, HttpRequest, HttpResponse>::ptr;
    explicit HttpServer(HttpServerConfig::ptr config);

    template <HttpMethod ...Methods>
    void RouteHandler(const std::string& path, std::function<Coroutine<void>(RoutineCtx,SessionPtr)>&& handler);
    void Start(THost host);
    void Stop();
    bool IsRunning() const;
private:
    Coroutine<void> HttpRouteForward(RoutineCtx ctx, typename Session<SocketType, HttpRequest, HttpResponse>::ptr session);
    void CreateHttpResponse(HttpResponse* response, HttpVersion version, HttpStatusCode code, std::string&& body);
private:
    TcpServer<SocketType> m_server;
};

template<typename SocketType>
extern Coroutine<void> HttpRoute(RoutineCtx ctx, size_t max_header_size, typename Session<SocketType, HttpRequest, HttpResponse>::ptr session);


template<typename SocketType>
extern Coroutine<std::string> Handle(RoutineCtx ctx, http::HttpMethod method, const std::string& path,\
                                        typename Session<SocketType, http::HttpRequest, http::HttpResponse>::ptr session, \
                                        typename HttpRouteHandler<SocketType>::HandlerMap& handlerMap);

}

#include "Server.tcc"

#endif