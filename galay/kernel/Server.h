#ifndef GALAY_SERVER_H
#define GALAY_SERVER_H

#include <string>
#include <vector>
#include <atomic>
#include <string_view>
#include <functional>
#include <unordered_map>
#include "concurrentqueue/moodycamel/concurrentqueue.h"
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
    class TcpCallbackStore;
    class TcpSslCallbackStore;
    class HttpOperation;
}

namespace galay::coroutine
{
    class Coroutine;
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

    struct TcpSslServerConfig final : public TcpServerConfig
    {
        using ptr = std::shared_ptr<TcpSslServerConfig>;
        TcpSslServerConfig();
        const char* m_cert_file;    //.crt 文件
        const char* m_key_file;     //.key 文件
        ~TcpSslServerConfig() override = default;
    };

    struct HttpServerConfig: public TcpServerConfig
    {
        using ptr = std::shared_ptr<HttpServerConfig>;
        HttpServerConfig();
        uint32_t m_proto_capacity{};          // 协议对象池容量
        uint32_t m_max_header_size;

        ~HttpServerConfig() override = default;
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
        inline bool IsRunning() { return m_is_running; }
        virtual ~TcpServer();
    protected:
        TcpServerConfig::ptr m_config;
        std::atomic_bool m_is_running;
        std::vector<event::ListenEvent*> m_listen_events;
    };

    class TcpSslServer final : public Server
    {
    public:
        explicit TcpSslServer(const TcpSslServerConfig::ptr& config);
        //no block
        void Start(TcpSslCallbackStore* store, int port);
        void Stop();
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

#define DEFAULT_PROTOCOL_CAPACITY               1024
#define DEFAULT_HTTP_KEEPALIVE_TIME_MS          (7500 * 1000)

    class HttpServer final : public TcpServer
    {
        static protocol::http::HttpResponse UriTooLongResponse;
        static protocol::http::HttpResponse NotFoundResponse;
        static protocol::http::HttpResponse MethodNotAllowedResponse;
    public:
        explicit HttpServer(const HttpServerConfig::ptr& config);

        static protocol::http::HttpResponse& GetDefaultMethodNotAllowedResponse();
        static protocol::http::HttpResponse& GetDefaultUriTooLongResponse();
        static protocol::http::HttpResponse& GetDefaultNotFoundResponse();
        
        //not thread security
        void Get(const std::string& path, std::function<coroutine::Coroutine(HttpOperation)>&& handler);
        void Post(const std::string& path, std::function<coroutine::Coroutine(HttpOperation)>&& handler);
        void Put(const std::string& path, std::function<coroutine::Coroutine(HttpOperation)>&& handler);
        void Start(int port);
        void Stop() override;
    private:
        coroutine::Coroutine HttpRoute(TcpConnectionManager operation);
    private:
        std::unique_ptr<TcpCallbackStore> m_store;
        std::unordered_map<protocol::http::HttpMethod, std::unordered_map<std::string, std::function<coroutine::Coroutine(HttpOperation)>>> m_route_map;
    };

    class HttpspServer
    {
    public:
    };
}

#endif