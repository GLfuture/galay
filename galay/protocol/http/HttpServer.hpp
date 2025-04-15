#ifndef GALAY_HTTP_SERVER_HPP
#define GALAY_HTTP_SERVER_HPP

#include "galay/kernel/Server.hpp"
#include "HttpMiddleware.hpp"
#include "HttpFormData.hpp"
#include "HttpRoute.hpp"
#include <unordered_set>
#include <string_view>
#include <filesystem>
#include <concurrentqueue/moodycamel/blockingconcurrentqueue.h>

namespace galay::http
{


class HttpHelper
{
public:
    using HttpResponseCode = HttpStatusCode;
    //request
    static bool DefaultGet(HttpRequest* request, const std::string& url, bool keepalive = true);
    static bool DefaultPost(HttpRequest* request, const std::string& url, bool keepalive = true);
    //response
    static bool DefaultRedirect(HttpResponse* response, const std::string& url, HttpResponseCode code);
    static bool DefaultOK(HttpResponse* response, HttpVersion version);

    static bool DefaultHttpResponse(HttpResponse* response, HttpVersion version, HttpStatusCode code, std::string type, std::string &&body);
};

template<HttpStatusCode Code>
class CodeResponse
{
public:
    static std::string ResponseStr(HttpVersion version, bool keepAlive);
    static bool RegisterResponse(HttpResponse response);
private:
    static std::string DefaultResponse(HttpVersion version, bool keepAlive);
    static std::string DefaultResponseBody() { return ""; }
private:
    static std::string m_responseStr;
};


template<HttpStatusCode Code>
std::string CodeResponse<Code>::m_responseStr = "";

struct HttpStreamConfig
{
    int32_t m_recv_timeout_ms = DEFAULT_HTTP_RECV_TIME_MS;
    int32_t m_send_timeout_ms = DEFAULT_HTTP_SEND_TIME_MS;
    uint32_t m_max_header_size = DEFAULT_HTTP_MAX_HEADER_SIZE;
};

template<typename SocketType>
class HttpStreamImpl;

template<typename SocketType>
Coroutine<bool> RecvHttpRequestImpl(RoutineCtx ctx\
    , typename HttpStreamImpl<SocketType>::ptr stream\
    , HttpRequest& request);

template<typename SocketType>
Coroutine<bool> SendHttpResponseImpl(RoutineCtx ctx\
    , typename HttpStreamImpl<SocketType>::ptr stream\
    , std::string&& response);

template<typename SocketType>
Coroutine<bool> SendStaticFileImpl(RoutineCtx ctx\
    , typename HttpStreamImpl<SocketType>::ptr stream\
    , FileDesc* desc);

template<typename SocketType>
Coroutine<void> Handle(RoutineCtx ctx\
    , typename HttpStreamImpl<SocketType>::ptr stream\
    , std::unordered_map<HttpMethod, HttpRouter>& routers\
    , HttpMiddleware* middleware);

class HttpStream
{
public:
    using HttpError = galay::error::HttpError;
    using ptr = std::shared_ptr<HttpStream>;

    virtual HttpError LastError() = 0;
    virtual HttpStreamConfig& GetConfig() = 0;

    virtual AsyncResult<bool, void> SendResponse(RoutineCtx ctx, HttpStatusCode code\
        , std::string&& content, const std::string& contentType = "") = 0;

    virtual AsyncResult<bool, void> SendResponse(RoutineCtx ctx, HttpResponse&& response) = 0;
    virtual AsyncResult<bool, void> SendFile(RoutineCtx ctx, FileDesc* desc) = 0;
    virtual AsyncResult<bool, void> Close() = 0;

    virtual bool Closed() = 0;
};

template<typename SocketType>
class HttpStreamImpl: public HttpStream, public std::enable_shared_from_this<HttpStreamImpl<SocketType>>
{
public:
    using ptr = std::shared_ptr<HttpStreamImpl>;
    using HttpError = galay::error::HttpError;
    using HttpStreamPtr = typename HttpStreamImpl<SocketType>::ptr;
    using Handler = std::function<Coroutine<void>(RoutineCtx,HttpStreamPtr)>;
    using HandlerMap = std::unordered_map<HttpMethod, std::unordered_map<std::string, Handler>>;

    template <typename T>
    friend Coroutine<bool> RecvHttpRequestImpl(RoutineCtx ctx, typename HttpStreamImpl<T>::ptr stream, HttpRequest& request);
    template<typename T>
    friend Coroutine<bool> SendHttpResponseImpl(RoutineCtx ctx, typename HttpStreamImpl<T>::ptr stream, std::string&& response);
    template<typename T>
    friend Coroutine<bool> SendStaticFileImpl(RoutineCtx ctx, typename HttpStreamImpl<T>::ptr stream, FileDesc* desc);
public:
    
    HttpStreamImpl(typename Connection<SocketType>::uptr connection, HttpStreamConfig conf);
    HttpError LastError() override;
    HttpStreamConfig& GetConfig() override;

    AsyncResult<bool, void> SendResponse(RoutineCtx ctx, HttpStatusCode code\
        , std::string&& content, const std::string& contentType = "") override;

    AsyncResult<bool, void> SendResponse(RoutineCtx ctx, HttpResponse&& response) override;
    AsyncResult<bool, void> SendFile(RoutineCtx ctx, FileDesc* desc) override;
    AsyncResult<bool, void> Close() override;

    bool Closed() override;
private:
    bool m_close = false;
    HttpError m_error;
    HttpStreamConfig m_config;
    typename Connection<SocketType>::uptr m_connection;
};

struct HttpServerConfig final: public TcpServerConfig
{
    using ptr = std::shared_ptr<HttpServerConfig>;
    static HttpServerConfig::ptr Create();
    ~HttpServerConfig() = default;

    int32_t m_recv_timeout_ms = DEFAULT_HTTP_RECV_TIME_MS;
    int32_t m_send_timeout_ms = DEFAULT_HTTP_SEND_TIME_MS;
    uint32_t m_max_header_size = DEFAULT_HTTP_MAX_HEADER_SIZE;
};

class HttpContext final
{
    template<typename SocketType>
    friend Coroutine<void> Handle(RoutineCtx ctx, typename HttpStreamImpl<SocketType>::ptr stream, std::unordered_map<HttpMethod, HttpRouter>& routers, HttpMiddleware* middleware);
public:
    HttpContext(HttpRequest&& request, HttpStream::ptr stream)
        : m_request(std::move(request)), m_stream(stream) {}
    HttpRequest& GetRequest() {     return m_request;   }
    HttpStream::ptr GetStream() {   return m_stream;    }

    std::string GetParam(const std::string& key)
    {
        if(m_params.find(key) != m_params.end()) {
            return m_params[key];
        }
        return "";
    }

    bool AddParam(const std::string& key, const std::string& value)
    {
        if(m_params.find(key) == m_params.end()) {
            m_params[key] = value;
            return true;
        }
        return false;
    }

    bool AddParam(std::unordered_map<std::string, std::string>&& params)
    {
        m_params = std::move(params);
        return true;
    }

private:
    HttpRequest m_request;
    HttpStream::ptr m_stream;
    std::unordered_map<std::string, std::string> m_params;
};

template<typename SocketType>
class HttpServer
{
public:
    using Handler = std::function<Coroutine<void>(RoutineCtx,HttpContext)>;

    explicit HttpServer(HttpServerConfig::ptr config);

    template <HttpMethod ...Methods>
    void RouteHandler(const std::string& path, Handler&& handler);

    
    void Start(THost host);
    void Stop();
    bool IsRunning() const;
    bool RegisterMiddleware(HttpMiddleware::uptr middleware);
    /*
     * 注册静态文件 GET 请求处理回调
     * @param url_path_prefix   URL 路径前缀（如 "/static"）
     * @param filesystem_path   对应的本地文件系统路径（如 "/var/www/static"）
     */
    bool RegisterStaticFileGetMiddleware(const std::string& url_path_prefix, const std::string& filesystem_path);

private:
    Coroutine<void> HttpRouteForward(RoutineCtx ctx, typename HttpStreamImpl<SocketType>::ptr stream);
private:
    TcpServer<SocketType> m_server;
    HttpMiddleware::uptr m_middleware;
    std::unordered_map<HttpMethod, HttpRouter> m_routers;
};



}

namespace galay
{

#define GET HttpMethod::Http_Method_Get
#define POST HttpMethod::Http_Method_Post
#define PUT HttpMethod::Http_Method_Put
#define DELETE HttpMethod::Http_Method_Delete
#define HEAD HttpMethod::Http_Method_Head
#define OPTIONS HttpMethod::Http_Method_Options
#define CONNECT HttpMethod::Http_Method_Connect
#define TRACE HttpMethod::Http_Method_Trace


#define HTTP_1_0 HttpVersion::Http_Version_1_0
#define HTTP_1_1 HttpVersion::Http_Version_1_1
#define HTTP_2_0 HttpVersion::Http_Version_2_0


}

#include "HttpServer.tcc"

#endif