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

Coroutine<void> CreateConnection(RoutineCtx ctx, galay::AsyncTcpSocket* socket, CallbackStore<galay::AsyncTcpSocket> *store, EventEngine *engine);
Coroutine<void> CreateConnection(RoutineCtx ctx, AsyncTcpSslSocket* socket, CallbackStore<AsyncTcpSslSocket> *store, EventEngine *engine);



template <typename SocketType>
class ListenEvent: public Event
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
    void CreateConnection(RoutineCtx ctx, EventEngine* engine);
private:
    SocketType* m_socket;
    CoroutineScheduler* m_scheduler;
    std::atomic<EventEngine*> m_engine;
    CallbackStore<SocketType>* m_store;
};



};

namespace galay
{
using HttpSession = Session<AsyncTcpSocket, protocol::http::HttpRequest, protocol::http::HttpResponse>;
using HttpsSession = Session<AsyncTcpSslSocket, protocol::http::HttpRequest, protocol::http::HttpResponse>;
}

namespace galay::server 
{

#define DEFAULT_SERVER_BACKLOG                          32
#define DEFAULT_COROUTINE_SCHEDULER_CONF                {4, -1}
#define DEFAULT_NETWORK_SCHEDULER_CONF                  {4, -1}
#define DEFAULT_TIMER_SCHEDULER_CONF                    {1, -1}


#define DEFAULT_HTTP_MAX_HEADER_SIZE                    4096
    
struct TcpServerConfig
{
    using ptr = std::shared_ptr<TcpServerConfig>;
    TcpServerConfig()
        : m_backlog(DEFAULT_SERVER_BACKLOG), m_coroutineConf(DEFAULT_COROUTINE_SCHEDULER_CONF), \
        m_netSchedulerConf(DEFAULT_NETWORK_SCHEDULER_CONF), m_timerSchedulerConf(DEFAULT_TIMER_SCHEDULER_CONF)
    {
    }

    int m_backlog;                                  // 半连接队列长度
    std::pair<uint32_t, int> m_coroutineConf;       // 协程调度器配置
    std::pair<uint32_t, int> m_netSchedulerConf;    // 网络调度器配置
    std::pair<uint32_t, int> m_timerSchedulerConf;  // 定时调度器配置
    virtual ~TcpServerConfig() = default;
};

struct HttpServerConfig final: public TcpServerConfig
{
    using ptr = std::shared_ptr<HttpServerConfig>;
    HttpServerConfig() :m_max_header_size(DEFAULT_HTTP_MAX_HEADER_SIZE) {}
    uint32_t m_max_header_size;

    ~HttpServerConfig() override = default;
};

template<typename SocketType>
class TcpServer
{
public:
    explicit TcpServer(TcpServerConfig::ptr config) :m_config(config) {}
    //no block
    void Start(CallbackStore<SocketType>* store, const std::string& addr, int port);
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

#define DEFAULT_HTTP_REQUEST_POOL_SIZE               2048
#define DEFAULT_HTTP_RESPONSE_POOL_SIZE              2048
#define DEFAULT_HTTP_KEEPALIVE_TIME_MS          (7500 * 1000)

using namespace galay::protocol::http;

template<HttpStatusCode Code>
class CodeResponse
{
public:
    static std::string ResponseStr(HttpVersion version)
    {
        if(m_responseStr.empty()) {
            m_responseStr = DefaultResponse(version);
        }
        return m_responseStr;
    }

    static bool RegisterResponse(HttpResponse response)
    {
        static_assert(response.Header()->Code() == Code , "HttpStatusCode not match");
        m_responseStr = response.EncodePdu();
        return true;
    }
private:
    static std::string DefaultResponse(HttpVersion version) {
        HttpResponse response;
        response.Header()->Code() = Code;
        response.Header()->Version() = version;
        response.Header()->HeaderPairs().AddHeaderPair("Content-Type", "text/html");
        response.Header()->HeaderPairs().AddHeaderPair("Server", "galay");
        response.Header()->HeaderPairs().AddHeaderPair("Date", galay::GetCurrentGMTTimeString());
        response.Header()->HeaderPairs().AddHeaderPair("Connection", "close");
        response.Body() = DefaultResponseBody();
        return response.EncodePdu();
    }

    static std::string DefaultResponseBody()
    {
        return "";
    }

private:
    static std::string m_responseStr;
};


template<HttpStatusCode Code>
std::string CodeResponse<Code>::m_responseStr = "";

template<typename SocketType>
class HttpRouteHandler 
{
    using Session = galay::Session<SocketType, HttpRequest, HttpResponse>;
public:
    void AddHandler(HttpMethod method, const std::string& path, std::function<Coroutine<void>(Session)>&& handler)
    {
        m_handler_map[method][path] = std::move(handler);
    }
    
    static HttpRouteHandler<SocketType>* GetInstance()
    {
        if(m_instance == nullptr) {
            m_instance = std::make_unique<HttpRouteHandler<SocketType>>();
        }
        return m_instance.get();
    }

    std::string Handle(HttpMethod method, const std::string& path, Session session) {
        auto it = m_handler_map.find(method);
        auto version = session.GetRequest()->Header()->Version();
        if(it == m_handler_map.end()) {
            return CodeResponse<HttpStatusCode::MethodNotAllowed_405>::ResponseStr(version);
        }
        auto uriit = it->second.find(path);
        if(uriit != it->second.end()) {
            uriit->second(session);
            return session.GetResponse()->EncodePdu();
        } else {
            return CodeResponse<HttpStatusCode::NotFound_404>::ResponseStr(version);
        }
        return CodeResponse<HttpStatusCode::InternalServerError_500>::ResponseStr(version);
    }   

private:
    static std::unique_ptr<HttpRouteHandler<SocketType>> m_instance;
    std::unordered_map<HttpMethod, std::unordered_map<std::string, std::function<Coroutine<void>(Session)>>> m_handler_map;
};

template<typename SocketType>
std::unique_ptr<HttpRouteHandler<SocketType>> HttpRouteHandler<SocketType>::m_instance = nullptr;


template<typename SocketType>
class HttpServer
{
public:
    using Session = galay::Session<SocketType, HttpRequest, HttpResponse>;
private:
    static utils::ProtocolPool<HttpRequest> RequestPool;
    static utils::ProtocolPool<HttpResponse> ResponsePool;
public:
    explicit HttpServer(HttpServerConfig::ptr config): m_server(config), 
        m_store(std::make_unique<CallbackStore<SocketType>>([this](std::shared_ptr<Connection<SocketType>> connection)->Coroutine<void> {
            return HttpRoute(connection);
        })) {}

    template <HttpMethod ...Methods>
    void RouteHandler(const std::string& path, std::function<galay::Coroutine<void>(galay::Session<SocketType, HttpRequest, HttpResponse>)>&& handler)
    {
         ([&](){
            HttpRouteHandler<SocketType>::GetInstance()->AddHandler(Methods, path, std::move(handler));
        }(), ...);
    }

    void Start(const std::string& addr, int port) {
        m_server.Start(m_store.get(), addr, port);
    }
    void Stop() {
        m_server.Stop();
    }

    bool IsRunning() const { return m_server.IsRunning(); }
private:
    Coroutine<void> HttpRoute(std::shared_ptr<Connection<SocketType>> connection);
    void CreateHttpResponse(HttpResponse* response, HttpVersion version, HttpStatusCode code, std::string&& body)
    {
        helper::http::HttpHelper::DefaultHttpResponse(response, version, code, "text/html", std::move(body));
    }
private:
    TcpServer<SocketType> m_server;
    std::unique_ptr<CallbackStore<SocketType>> m_store;
};

template<typename SocketType>
inline utils::ProtocolPool<HttpRequest> HttpServer<SocketType>::RequestPool(DEFAULT_HTTP_REQUEST_POOL_SIZE);
template<typename SocketType>
inline utils::ProtocolPool<HttpResponse> HttpServer<SocketType>::ResponsePool(DEFAULT_HTTP_RESPONSE_POOL_SIZE);


}

#include "Server.tcc"

#endif