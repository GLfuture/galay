#ifndef GALAY_HTTP_TCC
#define GALAY_HTTP_TCC

#include "Http.hpp"

namespace galay::http
{
template <HttpStatusCode Code>
inline std::string CodeResponse<Code>::ResponseStr(HttpVersion version)
{
    if(m_responseStr.empty()) {
        m_responseStr = DefaultResponse(version);
    }
    return m_responseStr;
}

template <HttpStatusCode Code>
inline bool CodeResponse<Code>::RegisterResponse(HttpResponse response)
{
    static_assert(response.Header()->Code() == Code , "HttpStatusCode not match");
    m_responseStr = response.EncodePdu();
    return true;
}

template <HttpStatusCode Code>
inline std::string CodeResponse<Code>::DefaultResponse(HttpVersion version)
{
    HttpResponse response;
    response.Header()->Code() = Code;
    response.Header()->Version() = version;
    response.Header()->HeaderPairs().AddHeaderPair("Content-Type", "text/html");
    response.Header()->HeaderPairs().AddHeaderPair("Server", "galay");
    response.Header()->HeaderPairs().AddHeaderPair("Date", utils::GetCurrentGMTTimeString());
    response.Header()->HeaderPairs().AddHeaderPair("Connection", "close");
    response.SetContent("html", DefaultResponseBody());
    return response.EncodePdu();
}

template<>
inline std::string CodeResponse<HttpStatusCode::BadRequest_400>::DefaultResponseBody()
{
    return "<html>Bad Request</html>";
}


template<>
inline std::string CodeResponse<HttpStatusCode::NotFound_404>::DefaultResponseBody()
{
    return "<html>404 Not Found</html>";
}

template<>
inline std::string CodeResponse<HttpStatusCode::MethodNotAllowed_405>::DefaultResponseBody()
{
    return "<html>405 Method Not Allowed</html>";
}

template <>
inline std::string CodeResponse<HttpStatusCode::UriTooLong_414>::DefaultResponseBody()
{
    return "<html>Uri Too Long</html>";
}

template<>
inline std::string CodeResponse<HttpStatusCode::RequestTimeout_408>::DefaultResponseBody()
{
    return "<html>408 Request Timeout</html>";
}

template <>
inline std::string CodeResponse<HttpStatusCode::RequestHeaderFieldsTooLarge_431>::DefaultResponseBody()
{
    return "<html>Header Too Long</html>";
}

template<>
inline std::string CodeResponse<HttpStatusCode::InternalServerError_500>::DefaultResponseBody()
{
    return "<html>500 Internal Server Error</html>";
}

template <typename SocketType>
inline HttpStream<SocketType>::HttpStream(typename Connection<SocketType>::uptr connection)
    : m_connection(std::move(connection))
{
}

template <typename SocketType>
inline Coroutine<HttpRequest::uptr> HttpStream<SocketType>::WaitForRequest(RoutineCtx ctx)
{
    return Coroutine<HttpRequest::uptr>();
}

template <typename SocketType>
inline Coroutine<bool> HttpStream<SocketType>::WaitForResponse(RoutineCtx ctx, HttpResponse::uptr request)
{
    return Coroutine<bool>();
}

template <typename SocketType>
inline HttpServer<SocketType>::HttpServer(HttpServerConfig::ptr config)
    :m_server(config)
{
}

template <typename SocketType>
inline void HttpServer<SocketType>::Start(THost host)
{
    m_server.OnCall([this](RoutineCtx ctx, Connection<SocketType>::uptr connection) {
        auto stream = std::make_unique<HttpStream>(std::move(connection));
        return HttpRouteForward(ctx, std::move(stream));
    });
    m_server.Start(host);
}

template <typename SocketType>
inline void HttpServer<SocketType>::Stop()
{
    m_server.Stop();
}

template <typename SocketType>
inline bool HttpServer<SocketType>::IsRunning() const
{
    return m_server.IsRunning();
}

template <typename SocketType>
template <HttpMethod... Methods>
inline void HttpServer<SocketType>::RouteHandler(const std::string &path, Handler&& handler)
{
    ([&](){
        this->m_map[Methods][path] = std::move(handler);
    }(), ...);
}

template <typename SocketType>
inline Coroutine<void> HttpServer<SocketType>::HttpRouteForward(RoutineCtx ctx, typename HttpStream<SocketType>::uptr stream)
{
    return Handle(ctx, std::move(stream));
}

template <typename SocketType>
inline void HttpServer<SocketType>::CreateHttpResponse(HttpResponse *response, HttpVersion version, HttpStatusCode code, std::string &&body)
{
    http::HttpHelper::DefaultHttpResponse(response, version, code, "text/html", std::move(body));
}



}


#endif