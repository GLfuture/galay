#ifndef __GALAY_SERVER_H__
#define __GALAY_SERVER_H__

#include <string>
#include <memory>
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
    class TcpOperation;
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
#define DEFAULT_SERVER_CO_SCHEDULER_TIMEOUT_MS          -1
#define DEFAULT_SERVER_CO_SCHEDULER_NUM                 4
#define DEFAULT_SERVER_NET_SCHEDULER_TIMEOUT_MS         -1
#define DEFAULT_SERVER_OTHER_SCHEDULER_NUM              0
#define DEFAULT_SERVER_OTHER_SCHEDULER_TIMEOUT_MS       -1
#define DEFAULT_SERVER_TIMER_SCHEDULER_NUM              1
#define DEFAULT_SERVER_TIMER_SCHEDULER_TIMEOUT_MS       -1
    
    struct TcpServerConfig
    {
        TcpServerConfig();
        int m_backlog;                          // 半连接队列长度
        int m_coroutine_scheduler_num;          // 协程线程个数
        int m_network_scheduler_num;            // 网络线程个数
        int m_network_scheduler_timeout_ms;     // EventEngine返回一次的超时
        int m_event_scheduler_num;              // 其他线程个数
        int m_event_scheduler_timeout_ms;       // EventEngine返回一次的超时
        int m_timer_scheduler_num;              // 定时线程个数
        int m_timer_scheduler_timeout_ms;       // EventEngine返回一次的超时
    };

    struct TcpSslServerConfig: public TcpServerConfig
    {
        TcpSslServerConfig();
        const char* m_cert_file;    //.crt 文件
        const char* m_key_file;     //.key 文件
    };

    class Server
    {
    public:
    };

    class TcpServer: public Server
    {
    public:
        TcpServer();
        TcpServer(const TcpServerConfig& config);
        //no block
        void Start(TcpCallbackStore* store, int port);
        void Stop();
        inline bool IsRunning() { return m_is_running; }
        virtual ~TcpServer();
    protected:
        TcpServerConfig m_config;
        std::atomic_bool m_is_running;
        std::vector<event::ListenEvent*> m_listen_events;
    };

    class TcpSslServer: public Server
    {
    public:
        TcpSslServer(const char* cert_file, const char* key_file);
        TcpSslServer(const TcpSslServerConfig& config);
        //no block
        void Start(TcpSslCallbackStore* store, int port);
        void Stop();
        inline bool IsRunning() { return m_is_running; }
        virtual ~TcpSslServer();
    protected:
        TcpSslServerConfig m_config;
        std::atomic_bool m_is_running;
        std::vector<event::SslListenEvent*> m_listen_events;
    };
    
    template<typename T>
    concept ProtoType = std::default_initializable<T> && requires(T t, const std::string_view& buffer)
    {
        { t.DecodePdu(buffer) } -> std::same_as<int>;
        { t.EncodePdu() }-> std::same_as<std::string>;
        { t.HasError() } -> std::same_as<bool>;
        { t.GetErrorCode() } -> std::same_as<int>;
        { t.GetErrorString() } -> std::same_as<std::string>;
        { t.Reset() } -> std::same_as<void>;
    };
    
    //keep protocol's num fixed
    template<ProtoType Request, ProtoType Response>
    class ServerProtocolStore
    {
    public:

        void Initial(uint32_t capacity)
        {
            m_capacity.store(capacity);
            for(int i = 0; i < capacity; ++i) {
                m_request_queue.enqueue(new Request());
            }
            for(int i = 0; i < capacity; ++i) {
                m_response_queue.enqueue(new Response());
            }
        }

        Request* GetRequest()
        {
            Request* request = nullptr;
            if(m_request_queue.try_dequeue(request)){
                return request;
            }
            return new Request();
        }

        void ReturnRequest(Request* request)
        {
            request->Reset();
            if(m_request_queue.size_approx() >= m_capacity.load()){
                m_request_queue.enqueue(request);
            }else{
                delete request;
            }
        }

        Response* GetResponse()
        {
            Response* response = nullptr;
            if(m_response_queue.try_dequeue(response)){
                return response;
            }
            return new Response();
        }

        void ReturnResponse(Response* response)
        {
            response->Reset();
            if(m_response_queue.size_approx() >= m_capacity.load()){
                m_response_queue.enqueue(response);
            }else{
                delete response;
            }
        }

        ~ServerProtocolStore()
        {
            Request* request = nullptr;
            Response* response = nullptr;
            while(m_request_queue.try_dequeue(request)){
                delete request;
            }
            while(m_response_queue.try_dequeue(response)){
                delete response;
            }
        }
    private:
        std::atomic_uint32_t m_capacity;
        moodycamel::ConcurrentQueue<Request*> m_request_queue;
        moodycamel::ConcurrentQueue<Response*> m_response_queue;
    };

#define DEFAULT_PROTOCOL_CAPACITY 1024

    class HttpServer: public TcpServer
    {
        static ServerProtocolStore<protocol::http::HttpRequest, protocol::http::HttpResponse> g_http_proto_store;
        static protocol::http::HttpResponse g_method_not_allowed_resp;
        static protocol::http::HttpResponse g_uri_too_long_resp;
        static protocol::http::HttpResponse g_not_found_resp;
    public:
        HttpServer(uint32_t proto_capacity = DEFAULT_PROTOCOL_CAPACITY);
        
        static void ReturnResponse(protocol::http::HttpResponse* response);
        static void ReturnRequest(protocol::http::HttpRequest* request);

        static protocol::http::HttpResponse& GetDefaultMethodNotAllowedResponse();
        static protocol::http::HttpResponse& GetDefaultUriTooLongResponse();
        static protocol::http::HttpResponse& GetDefaultNotFoundResponse();
        
        //not thread security
        void Get(const std::string& path, std::function<coroutine::Coroutine(HttpOperation)>&& handler);
        void Post(const std::string& path, std::function<coroutine::Coroutine(HttpOperation)>&& handler);
        void Put(const std::string& path, std::function<coroutine::Coroutine(HttpOperation)>&& handler);
        void Start(int port);
        void Stop();
    private:
        static coroutine::Coroutine HttpRoute(TcpOperation operation);
    private:
        std::unique_ptr<TcpCallbackStore> m_store;
        static std::unordered_map<protocol::http::HttpMethod, std::unordered_map<std::string, std::function<coroutine::Coroutine(HttpOperation)>>> m_route_map;
    };

    class HttpspServer
    {
    public:
    };
}

#endif