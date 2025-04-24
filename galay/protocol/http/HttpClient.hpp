#ifndef GALAY_HTTP_CLIENT_HPP
#define GALAY_HTTP_CLIENT_HPP

#include "galay/kernel/Client.hpp"
#include "HttpCommon.hpp"
#include "HttpProtoc.hpp"
#include <regex>

namespace galay::http
{

template<typename SocketType>
class HttpAbstractClient;

template<typename SocketType>
static Coroutine<bool> SendHttpRequestImpl(RoutineCtx ctx\
    , HttpAbstractClient<SocketType>* client\
    , HttpRequest &&request\
    , int64_t timeout);

template<typename SocketType>
static Coroutine<bool> RecvHttpResponseImpl(RoutineCtx ctx\
    , HttpAbstractClient<SocketType>* client\
    , HttpResponse* response, int64_t timeout);
    
template<typename SocketType>
static Coroutine<HttpResponse> Handle(RoutineCtx ctx\
    , HttpAbstractClient<SocketType>* client\
    , HttpRequest &&request\
    , int64_t timeout);

template<typename SocketType>
class HttpAbstractClient
{
    template<typename T>
    friend Coroutine<bool> SendHttpRequestImpl(RoutineCtx ctx\
        , HttpAbstractClient<T>* client\
        , HttpRequest &&request\
        , int64_t timeout);

    template<typename T>
    friend Coroutine<bool> RecvHttpResponseImpl(RoutineCtx ctx\
        , HttpAbstractClient<T>* client\
        , HttpResponse* response, int64_t timeout);
    
    template<typename T>
    friend Coroutine<HttpResponse> Handle(RoutineCtx ctx\
        , HttpAbstractClient<T>* client\
        , HttpRequest &&request\
        , int64_t timeout);

    using HttpError = error::HttpError;
public:
    using ptr = std::shared_ptr<HttpAbstractClient>;
    using uptr = std::unique_ptr<HttpAbstractClient>;

    HttpAbstractClient(GHandle handle = {});

    template<typename CoRtn = void>
    AsyncResult<bool, CoRtn> Connect(THost* addr, int64_t timeout = -1);
    template<typename CoRtn = void>
    AsyncResult<bool, CoRtn> Connect(const std::string& url, int64_t timeout = -1);

    template<typename CoRtn = void>
    AsyncResult<HttpResponse, CoRtn> Get(RoutineCtx ctx, const std::string& url, int64_t timeout, bool keepalive = true);

    template<typename CoRtn = void>
    AsyncResult<HttpResponse, CoRtn> Post(RoutineCtx ctx, const std::string& url, std::string&& content, std::string&& content_type, int64_t timeout, bool keepalive = true);

    template<typename CoRtn = void>
    AsyncResult<HttpResponse, CoRtn> Put(RoutineCtx ctx, const std::string& url, std::string &&content, std::string&& content_type, int64_t timeout, bool keepalive = true);
    
    template<typename CoRtn = void>
    AsyncResult<HttpResponse, CoRtn> Delete(RoutineCtx ctx, const std::string& url, std::string&& content, std::string&& content_type, int64_t timeout, bool keepalive = true);
    
    template<typename CoRtn = void>
    AsyncResult<bool, CoRtn> Close();

    std::string GetHttpErrorString();
    uint32_t GetErrorCode();

    THost GetRemoteAddr();
private:
    error::HttpError m_error;
    TcpClient<SocketType> m_client;

    int32_t m_max_header_size = gHttpMaxHeaderSize.load();
    std::chrono::steady_clock::time_point m_last_time;
};

using HttpClient = HttpAbstractClient<AsyncTcpSocket>;
using HttpsClient = HttpAbstractClient<AsyncTcpSslSocket>;

}

#include "HttpClient.tcc"

#endif