#ifndef GALAY_SERVER_H
#define GALAY_SERVER_H

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
    TcpServerConfig();
    int m_backlog;                                  // 半连接队列长度
    std::pair<uint32_t, int> m_coroutineConf;       // 协程调度器配置
    std::pair<uint32_t, int> m_netSchedulerConf;    // 网络调度器配置
    std::pair<uint32_t, int> m_timerSchedulerConf;  // 定时调度器配置
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

template<typename SocketType, typename ConfigType>
class Server
{
public:
    explicit Server(std::shared_ptr<ConfigType> config)
    {
        m_config = config;
    }
    //no block
    void Start(CallbackStore<SocketType>* store, const std::string& addr, int port) {
        m_is_running = true;
        m_listen_events.resize(m_config->m_netSchedulerConf.first);
        for(int i = 0 ; i < m_config->m_netSchedulerConf.first; ++i )
        {
            SocketType* socket = new SocketType(EeventSchedulerHolder::GetInstance()->GetScheduler(i)->GetEngine());
            if(const bool res = socket->Socket(); !res ) {
                delete socket;
                for(int j = 0; j < i; ++j ){
                    delete m_listen_events[j];
                }
                m_listen_events.clear();
                LogError("[SocketType Init failed, error:{}]", error::GetErrorString(socket->GetErrorCode()));
                return;
            }
            HandleOption option(socket->GetHandle());
            option.HandleReuseAddr();
            option.HandleReusePort();
            
            if(!socket->Bind(addr, port)) {
                LogError("[Bind failed, error:{}]", error::GetErrorString(socket->GetErrorCode()));
                delete socket;
                for(int j = 0; j < i; ++j ){
                    delete m_listen_events[j];
                }
                m_listen_events.clear();
                return;
            }
            if(!socket->Listen(m_config->m_backlog)) {
                LogError("[Listen failed, error:{}]", error::GetErrorString(socket->GetErrorCode()));
                delete socket;
                for(int j = 0; j < i; ++j ){
                    delete m_listen_events[j];
                }
                m_listen_events.clear();
                return;
            }
            m_listen_events[i] = new details::ListenEvent<SocketType>(EeventSchedulerHolder::GetInstance()->GetScheduler(i)->GetEngine(), socket, store);
        } 
    }
    void Stop() {
        if(!m_is_running) return;
        for(const auto & m_listen_event : m_listen_events)
        {
            delete m_listen_event;
        }
        m_is_running = false;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::weak_ptr<ConfigType> GetConfig() { return m_config; }
    inline bool IsRunning() { return m_is_running; }
    ~Server() = default;
protected:
    std::shared_ptr<ConfigType> m_config;
    std::atomic_bool m_is_running;
    std::vector<details::ListenEvent<SocketType>*> m_listen_events;
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

template<typename SocketType, typename ConfigType>
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
    explicit HttpServer(std::shared_ptr<ConfigType> config) 
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
    Coroutine HttpRoute(std::shared_ptr<Connection<SocketType>> connection)
    {
        co_await this_coroutine::AddToCoroutineStore();
        size_t max_header_size = m_server.GetConfig().lock()->m_max_header_size;
        AsyncTcpSocket* socket = connection->GetSocket();
        IOVecHolder<TcpIOVec> rholder(max_header_size), wholder;
        Session session(connection, &RequestPool, &ResponsePool);
        auto [context, cancel] = coroutine::ContextFactory::WithWaitGroupContext();
        co_await this_coroutine::DeferExit(std::bind([](coroutine::RoutineContextCancel::ptr cancel){
            (*cancel)();
        }, cancel));
        bool close_connection = false;
        while(true)
        {
    step1:
            while(true) {
                int length = co_await socket->Recv(&rholder, max_header_size);
                if( length <= 0 ) {
                    if( length == details::CommonFailedType::eCommonDisConnect || length == details::CommonFailedType::eCommonOtherFailed ) {
                        bool res = co_await socket->Close();
                        co_return;
                    }
                } 
                else { 
                    std::string_view data(rholder->m_buffer, rholder->m_offset);
                    auto result = session.GetRequest()->DecodePdu(data);
                    if(!result.first) { //解析失败
                        switch (session.GetRequest()->GetErrorCode())
                        {
                        case error::HttpErrorCode::kHttpError_HeaderInComplete:
                        case error::HttpErrorCode::kHttpError_BodyInComplete:
                        {
                            if( rholder->m_offset >= rholder->m_size) {
                                rholder.Realloc(rholder->m_size * 2);
                            }
                            break;
                        }
                        case galay::error::HttpErrorCode::kHttpError_BadRequest:
                        {
                            if(m_error_string.contains(HttpStatusCode::BadRequest_400)) {
                                std::string body = m_error_string[HttpStatusCode::BadRequest_400];
                                CreateHttpResponse(session.GetResponse(), HttpVersion::Http_Version_1_1, HttpStatusCode::BadRequest_400, std::move(body));
                            } else {
                                std::string body = "Bad Request";
                                CreateHttpResponse(session.GetResponse(), HttpVersion::Http_Version_1_1, HttpStatusCode::BadRequest_400, std::move(body));
                            }
                            wholder.Reset(session.GetResponse()->EncodePdu());
                            close_connection = true;
                            goto step2;
                        }
                        case galay::error::HttpErrorCode::kHttpError_HeaderTooLong:
                        {
                            if(m_error_string.contains(HttpStatusCode::RequestHeaderFieldsTooLarge_431)) {
                                std::string body = m_error_string[HttpStatusCode::RequestHeaderFieldsTooLarge_431];
                                CreateHttpResponse(session.GetResponse(), HttpVersion::Http_Version_1_1, HttpStatusCode::RequestHeaderFieldsTooLarge_431, std::move(body));
                            } else {
                                std::string body = "Header Too Long";
                                CreateHttpResponse(session.GetResponse(), HttpVersion::Http_Version_1_1, HttpStatusCode::RequestHeaderFieldsTooLarge_431, std::move(body));
                            }
                            wholder.Reset(session.GetResponse()->EncodePdu());
                            close_connection = true;
                            goto step2;
                        }
                        case galay::error::HttpErrorCode::kHttpError_UriTooLong:
                        {
                            if(m_error_string.contains(HttpStatusCode::UriTooLong_414)) {
                                std::string body = m_error_string[HttpStatusCode::UriTooLong_414];
                                CreateHttpResponse(session.GetResponse(), HttpVersion::Http_Version_1_1, HttpStatusCode::UriTooLong_414, std::move(body));
                            } else {
                                std::string body = "Uri Too Long";
                                CreateHttpResponse(session.GetResponse(), HttpVersion::Http_Version_1_1, HttpStatusCode::UriTooLong_414, std::move(body));
                            }
                            wholder.Reset(session.GetResponse()->EncodePdu());
                            close_connection = true;
                            goto step2;
                        }
                        default:
                            break;
                        }
                    }
                    else { //解析成功
                        auto it = m_route_map.find(session.GetRequest()->Header()->Method());
                        if(it != m_route_map.end()) {
                            auto it2 = it->second.find(session.GetRequest()->Header()->Uri());
                            if(it2 != it->second.end()) {
                                it2->second(session, context);
                                wholder.Reset(session.GetResponse()->EncodePdu());
                                goto step2;
                            } else { //没有该URI
                                if(m_error_string.contains(HttpStatusCode::NotFound_404)) {
                                    std::string body = m_error_string[HttpStatusCode::NotFound_404];
                                    CreateHttpResponse(session.GetResponse(), HttpVersion::Http_Version_1_1, HttpStatusCode::NotFound_404, std::move(body));
                                } else {
                                    std::string body = "Not Found";
                                    CreateHttpResponse(session.GetResponse(), HttpVersion::Http_Version_1_1, HttpStatusCode::NotFound_404, std::move(body));
                                }
                                wholder.Reset(session.GetResponse()->EncodePdu());
                                goto step2;
                            }
                        } else { //没有该方法
                            if(m_error_string.contains(HttpStatusCode::MethodNotAllowed_405)) {
                                std::string body = m_error_string[HttpStatusCode::MethodNotAllowed_405];
                                CreateHttpResponse(session.GetResponse(), HttpVersion::Http_Version_1_1, HttpStatusCode::MethodNotAllowed_405, std::move(body));
                            } else {
                                std::string body = "Method NotAllowed";
                                CreateHttpResponse(session.GetResponse(), HttpVersion::Http_Version_1_1, HttpStatusCode::MethodNotAllowed_405, std::move(body));
                            }
                            wholder.Reset(session.GetResponse()->EncodePdu());
                            goto step2;
                        }
                    }
                }
            }

    step2:
            while (true)
            {
                int length = co_await socket->Send(&wholder, wholder->m_size);
                if( length <= 0 ) {
                    close_connection = true;
                    break;
                } else {
                    if(wholder->m_offset >= wholder->m_size) {
                        break;
                    }
                }
            }
            if(session.GetResponse()->Header()->HeaderPairs().GetValue("Connection") == "Close" || session.GetResponse()->Header()->HeaderPairs().GetValue("Connection") == "close") {
                close_connection = true;
            }
            // clear
            rholder.ClearBuffer(), wholder.ClearBuffer();
            session.GetRequest()->Reset(), session.GetResponse()->Reset();
            if(close_connection) {
                bool res = co_await socket->Close();
                break;
            }
        }
        co_return;
    }
    void CreateHttpResponse(HttpResponse* response, HttpVersion version, HttpStatusCode code, std::string&& body)
    {
        helper::http::HttpHelper::DefaultHttpResponse(response, version, code, "text/html", std::move(body));
    }
private:
    Server<SocketType, ConfigType> m_server;
    std::unique_ptr<CallbackStore<SocketType>> m_store;
    std::unordered_map<HttpMethod, std::unordered_map<std::string, std::function<Coroutine(Session, RoutineContext_ptr)>>> m_route_map;
    std::unordered_map<HttpStatusCode, std::string> m_error_string;
};

template<typename SocketType, typename ConfigType>
inline utils::ObjectPoolMutiThread<typename HttpServer<SocketType, ConfigType>::HttpRequest> HttpServer<SocketType, ConfigType>::RequestPool(DEFAULT_HTTP_REQUEST_POOL_SIZE);
template<typename SocketType, typename ConfigType>
inline utils::ObjectPoolMutiThread<typename HttpServer<SocketType, ConfigType>::HttpResponse> HttpServer<SocketType, ConfigType>::ResponsePool(DEFAULT_HTTP_RESPONSE_POOL_SIZE);


}
#endif