#ifndef GALAY_SERVER_H
#define GALAY_SERVER_H

#include <string>
#include <vector>
#include <atomic>
#include <string_view>
#include <functional>
#include <unordered_map>
#include <concurrentqueue/moodycamel/concurrentqueue.h>
#include "galay/util/ObjectPool.hpp"
#include "galay/protocol/Http.h"

namespace galay::event
{
    class ListenEvent;
    class SslListenEvent;
};

namespace galay::async
{
    class AsyncNetIo;
    class AsyncSslNetIo;
}

namespace galay
{
    class TcpConnectionManager;
    class TcpSslConnectionManager;
    class TcpCallbackStore;
    class TcpSslCallbackStore;
    class HttpConnectionManager;
    class HttpsConnectionManager;
}

namespace galay::coroutine
{
    class Coroutine;
    class RoutineContext;
}


namespace galay::server 
{

#define DEFAULT_SERVER_BACKLOG                          32
#define DEFAULT_SERVER_NET_SCHEDULER_NUM                4
#define DEFAULT_SERVER_CO_SCHEDULER_TIMEOUT_MS          (-1)
#define DEFAULT_SERVER_CO_SCHEDULER_NUM                 4
#define DEFAULT_SERVER_NET_SCHEDULER_TIMEOUT_MS         (-1)
#define DEFAULT_SERVER_TIMER_SCHEDULER_NUM              1
#define DEFAULT_SERVER_TIMER_SCHEDULER_TIMEOUT_MS       (-1)


#define DEFAULT_HTTP_MAX_HEADER_SIZE                    4096
    
    struct TcpServerConfig
    {
        using ptr = std::shared_ptr<TcpServerConfig>;
        TcpServerConfig();
        int m_backlog;                          // 半连接队列长度
        int m_coroutine_scheduler_num;          // 协程线程个数
        int m_network_scheduler_num;            // 网络线程个数
        int m_network_scheduler_timeout_ms;     // EventEngine返回一次的超时
        int m_timer_scheduler_num;              // 定时线程个数
        int m_timer_scheduler_timeout_ms;       // EventEngine返回一次的超时
        virtual ~TcpServerConfig() = default;
    };

    struct TcpSslServerConfig: public TcpServerConfig
    {
        using ptr = std::shared_ptr<TcpSslServerConfig>;
        TcpSslServerConfig();
        const char* m_cert_file;    //.crt 文件
        const char* m_key_file;     //.key 文件
        ~TcpSslServerConfig() override = default;
    };

    struct HttpServerConfig final: public TcpServerConfig
    {
        using ptr = std::shared_ptr<HttpServerConfig>;
        HttpServerConfig();
        uint32_t m_max_header_size;

        ~HttpServerConfig() override = default;
    };

    struct HttpsServerConfig final: public TcpSslServerConfig 
    {
        using ptr = std::shared_ptr<HttpsServerConfig>;
        HttpsServerConfig();
        uint32_t m_max_header_size;
        ~HttpsServerConfig() override = default;
    };

    class Server
    {
    public:
    };

    class TcpServer: public Server
    {
    public:
        explicit TcpServer(TcpServerConfig::ptr config);
        //no block
        void Start(TcpCallbackStore* store, int port);
        virtual void Stop();
        TcpServerConfig::ptr GetConfig() { return m_config; }
        inline bool IsRunning() { return m_is_running; }
        virtual ~TcpServer();
    protected:
        TcpServerConfig::ptr m_config;
        std::atomic_bool m_is_running;
        std::vector<event::ListenEvent*> m_listen_events;
    };

    class TcpSslServer: public Server
    {
    public:
        explicit TcpSslServer(const TcpSslServerConfig::ptr& config);
        //no block
        void Start(TcpSslCallbackStore* store, int port);
        void Stop();
        TcpSslServerConfig::ptr GetConfig() { return m_config; }
        bool IsRunning() { return m_is_running; }
        ~TcpSslServer();
    protected:
        TcpSslServerConfig::ptr m_config;
        std::atomic_bool m_is_running;
        std::vector<event::SslListenEvent*> m_listen_events;
    };
    
    template<typename T>
    concept ProtoType = std::default_initializable<T> && requires(T t, const std::string_view& buffer)
    {
        { t.DecodePdu(buffer) } -> std::same_as<std::pair<bool, size_t>>;
        { t.EncodePdu() }-> std::same_as<std::string>;
        { t.HasError() } -> std::same_as<bool>;
        { t.GetErrorCode() } -> std::same_as<int>;
        { t.GetErrorString() } -> std::same_as<std::string>;
        { t.Reset() } -> std::same_as<void>;
    };

#define DEFAULT_HTTP_REQUEST_POOL_SIZE               2048
#define DEFAULT_HTTP_RESPONSE_POOL_SIZE              2048
#define DEFAULT_HTTP_KEEPALIVE_TIME_MS          (7500 * 1000)

    class HttpServer final: public Server
    {
        using HttpRequest = protocol::http::HttpRequest;
        using HttpResponse = protocol::http::HttpResponse;
        using HttpVersion = protocol::http::HttpVersion;
        using HttpMethod = protocol::http::HttpMethod;
        using HttpStatusCode = protocol::http::HttpStatusCode;

        using Coroutine = coroutine::Coroutine;
        using RoutineContext_ptr = std::shared_ptr<coroutine::RoutineContext>;

        static util::ObjectPoolMutiThread<HttpRequest> RequestPool;
        static util::ObjectPoolMutiThread<HttpResponse> ResponsePool;
    public:
        explicit HttpServer(const HttpServerConfig::ptr& config);
        //not thread security
        void Get(const std::string& path, std::function<Coroutine(HttpConnectionManager, RoutineContext_ptr)>&& handler);
        void Post(const std::string& path, std::function<Coroutine(HttpConnectionManager, RoutineContext_ptr)>&& handler);
        void Put(const std::string& path, std::function<Coroutine(HttpConnectionManager, RoutineContext_ptr)>&& handler);
        void Delete(const std::string& path, std::function<Coroutine(HttpConnectionManager, RoutineContext_ptr)>&& handler);
        void Options(const std::string& path, std::function<Coroutine(HttpConnectionManager, RoutineContext_ptr)>&& handler);
        void Head(const std::string& path, std::function<Coroutine(HttpConnectionManager, RoutineContext_ptr)>&& handler);
        void Trace(const std::string& path, std::function<Coroutine(HttpConnectionManager, RoutineContext_ptr)>&& handler);
        void Patch(const std::string& path, std::function<Coroutine(HttpConnectionManager, RoutineContext_ptr)>&& handler);
        void Connect(const std::string& path, std::function<Coroutine(HttpConnectionManager, RoutineContext_ptr)>&& handler);

        void Start(int port);
        void Stop();
    private:
        Coroutine HttpRoute(TcpConnectionManager manager);
        void CreateHttpResponse(HttpResponse* response, HttpVersion version, HttpStatusCode code, std::string&& body);
    private:
        TcpServer m_server;
        std::unique_ptr<TcpCallbackStore> m_store;
        std::unordered_map<HttpMethod, std::unordered_map<std::string, std::function<Coroutine(HttpConnectionManager, RoutineContext_ptr)>>> m_route_map;
        std::unordered_map<HttpStatusCode, std::string> m_error_string;
    };

    class HttpsServer: public Server
    {
        using HttpRequest = protocol::http::HttpRequest;
        using HttpResponse = protocol::http::HttpResponse;
        using HttpVersion = protocol::http::HttpVersion;
        using HttpMethod = protocol::http::HttpMethod;
        using HttpStatusCode = protocol::http::HttpStatusCode;

        using Coroutine = coroutine::Coroutine;
        using RoutineContext_ptr = std::shared_ptr<coroutine::RoutineContext>;

        static util::ObjectPoolMutiThread<HttpRequest> RequestPool;
        static util::ObjectPoolMutiThread<HttpResponse> ResponsePool;
    public:
        explicit HttpsServer(const HttpsServerConfig::ptr& config);
        //not thread security
        void Get(const std::string& path, std::function<Coroutine(HttpsConnectionManager, RoutineContext_ptr)>&& handler);
        void Post(const std::string& path, std::function<Coroutine(HttpsConnectionManager, RoutineContext_ptr)>&& handler);
        void Put(const std::string& path, std::function<Coroutine(HttpsConnectionManager, RoutineContext_ptr)>&& handler);
        void Delete(const std::string& path, std::function<Coroutine(HttpsConnectionManager, RoutineContext_ptr)>&& handler);
        void Options(const std::string& path, std::function<Coroutine(HttpsConnectionManager, RoutineContext_ptr)>&& handler);
        void Head(const std::string& path, std::function<Coroutine(HttpsConnectionManager, RoutineContext_ptr)>&& handler);
        void Trace(const std::string& path, std::function<Coroutine(HttpsConnectionManager, RoutineContext_ptr)>&& handler);
        void Patch(const std::string& path, std::function<Coroutine(HttpsConnectionManager, RoutineContext_ptr)>&& handler);
        void Connect(const std::string& path, std::function<Coroutine(HttpsConnectionManager, RoutineContext_ptr)>&& handler);

        void Start(int port);
        void Stop();
    private:
        Coroutine HttpRoute(TcpSslConnectionManager manager);
        void CreateHttpResponse(HttpResponse* response, HttpVersion version, HttpStatusCode code, std::string&& body);
    private:
        TcpSslServer m_server;
        std::unique_ptr<TcpSslCallbackStore> m_store;
        std::unordered_map<HttpMethod, std::unordered_map<std::string, std::function<Coroutine(HttpsConnectionManager, RoutineContext_ptr)>>> m_route_map;
        std::unordered_map<HttpStatusCode, std::string> m_error_string;
    };
}

#endif