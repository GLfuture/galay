#ifndef GALAY_SERVER_HPP
#define GALAY_SERVER_HPP

#include <string>
#include <vector>
#include <atomic>
#include <string_view>
#include <unordered_map>
#include <concurrentqueue/moodycamel/concurrentqueue.h>
#include "Event.h"
#include "galay/helper/HttpHelper.h"

namespace galay::details
{
    template<typename SocketType>
    class ListenEvent;
};


namespace galay::coroutine
{
    class Coroutine;
    class RoutineContext;
}

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

template<typename SocketType>
class HttpServer
{
public:
    using HttpRequest = protocol::http::HttpRequest;
    using HttpResponse = protocol::http::HttpResponse;
    using HttpVersion = protocol::http::HttpVersion;
    using HttpMethod = protocol::http::HttpMethod;
    using HttpStatusCode = protocol::http::HttpStatusCode;

    using Coroutine = coroutine::Coroutine;
    using RoutineContext_ptr = std::shared_ptr<coroutine::RoutineContext>;
    
    using Session = galay::Session<SocketType, protocol::http::HttpRequest, protocol::http::HttpResponse>;
    
private:
    static utils::ObjectPoolMutiThread<HttpRequest> RequestPool;
    static utils::ObjectPoolMutiThread<HttpResponse> ResponsePool;
public:
    explicit HttpServer(HttpServerConfig::ptr config) 
    : m_server(config), m_store(std::make_unique<CallbackStore<SocketType>>([this](std::shared_ptr<Connection<SocketType>> connection)->coroutine::Coroutine {
		return HttpRoute(connection);
	})) {}
    //not thread security
    void Get(const std::string& path, std::function<Coroutine(Session, RoutineContext_ptr)>&& handler) 
    {
	    m_route_map[HttpMethod::Http_Method_Get][path] = std::move(handler);
    }
    void Post(const std::string& path, std::function<Coroutine(Session, RoutineContext_ptr)>&& handler)
    {
        m_route_map[HttpMethod::Http_Method_Post][path] = std::move(handler);
    }

    void Put(const std::string& path, std::function<Coroutine(Session, RoutineContext_ptr)>&& handler)
    {
        m_route_map[HttpMethod::Http_Method_Put][path] = std::move(handler);
    }

    void Delete(const std::string& path, std::function<Coroutine(Session, RoutineContext_ptr)>&& handler)
    {
        m_route_map[HttpMethod::Http_Method_Delete][path] = std::move(handler);
    }

    void Options(const std::string& path, std::function<Coroutine(Session, RoutineContext_ptr)>&& handler)
    {
        m_route_map[HttpMethod::Http_Method_Options][path] = std::move(handler);
    }

    void Head(const std::string& path, std::function<Coroutine(Session, RoutineContext_ptr)>&& handler)
    {
        m_route_map[HttpMethod::Http_Method_Head][path] = std::move(handler);
    }

    void Trace(const std::string& path, std::function<Coroutine(Session, RoutineContext_ptr)>&& handler)
    {
        m_route_map[HttpMethod::Http_Method_Trace][path] = std::move(handler);
    }

    void Patch(const std::string& path, std::function<Coroutine(Session, RoutineContext_ptr)>&& handler)
    {
        m_route_map[HttpMethod::Http_Method_Patch][path] = std::move(handler);
    }

    void Connect(const std::string& path, std::function<Coroutine(Session, RoutineContext_ptr)>&& handler)
    {
        m_route_map[HttpMethod::Http_Method_Connect][path] = std::move(handler);
    }

    void Start(const std::string& addr, int port) {
        m_server.Start(m_store.get(), addr, port);
    }
    void Stop() {
        m_server.Stop();
    }

    bool IsRunning() const { return m_server.IsRunning(); }
private:
    Coroutine HttpRoute(std::shared_ptr<Connection<SocketType>> connection);
    void CreateHttpResponse(HttpResponse* response, HttpVersion version, HttpStatusCode code, std::string&& body)
    {
        helper::http::HttpHelper::DefaultHttpResponse(response, version, code, "text/html", std::move(body));
    }
private:
    TcpServer<SocketType> m_server;
    std::unique_ptr<CallbackStore<SocketType>> m_store;
    std::unordered_map<HttpMethod, std::unordered_map<std::string, std::function<Coroutine(Session, RoutineContext_ptr)>>> m_route_map;
    std::unordered_map<HttpStatusCode, std::string> m_error_string;
};

template<typename SocketType>
inline utils::ObjectPoolMutiThread<typename HttpServer<SocketType>::HttpRequest> HttpServer<SocketType>::RequestPool(DEFAULT_HTTP_REQUEST_POOL_SIZE);
template<typename SocketType>
inline utils::ObjectPoolMutiThread<typename HttpServer<SocketType>::HttpResponse> HttpServer<SocketType>::ResponsePool(DEFAULT_HTTP_RESPONSE_POOL_SIZE);

}

#include "Server.tcc"

#endif