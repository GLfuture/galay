#ifndef GALAY_HTTP_TCC
#define GALAY_HTTP_TCC

#include "Http.hpp"

#define SERVER_NAME "galay"
#define GALAY_SERVER SERVER_NAME "/" GALAY_VERSION

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
    response.Header()->HeaderPairs().AddHeaderPair("Server", GALAY_SERVER);
    response.Header()->HeaderPairs().AddHeaderPair("Date", utils::GetCurrentGMTTimeString());
    response.Header()->HeaderPairs().AddHeaderPair("Connection", "close");
    response.SetContent("html", DefaultResponseBody());
    return response.EncodePdu();
}

template<>
inline std::string CodeResponse<HttpStatusCode::BadRequest_400>::DefaultResponseBody()
{
    return "<html>400 Bad Request</html>";
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
    return "<html>414 Uri Too Long</html>";
}

template<>
inline std::string CodeResponse<HttpStatusCode::RequestTimeout_408>::DefaultResponseBody()
{
    return "<html>408 Request Timeout</html>";
}

template <>
inline std::string CodeResponse<HttpStatusCode::RequestHeaderFieldsTooLarge_431>::DefaultResponseBody()
{
    return "<html>431 Header Too Long</html>";
}

template<>
inline std::string CodeResponse<HttpStatusCode::InternalServerError_500>::DefaultResponseBody()
{
    return "<html>500 Internal Server Error</html>";
}

template <typename SocketType>
inline HttpStream<SocketType>::HttpStream(typename Connection<SocketType>::uptr connection, HttpStreamConfig conf, HandlerMap& map)
    : m_connection(std::move(connection)), m_config(conf), m_handlerMap(map)
{
}

template <typename SocketType>
inline HttpStream<SocketType>::HttpError HttpStream<SocketType>::LastError()
{
    return m_error;
}

template <typename SocketType>
inline HttpStreamConfig &HttpStream<SocketType>::GetConfig()
{
    return m_config;
}

template <typename SocketType>
inline bool HttpStream<SocketType>::Closed()
{
    return m_close;
}

template <typename SocketType>
inline HttpRequest::uptr &HttpStream<SocketType>::GetRequest()
{
    return m_request;
}

template <typename SocketType>
inline AsyncResult<bool, void> HttpStream<SocketType>::SendResponse(RoutineCtx ctx, HttpStatusCode code, std::string &&content, const std::string &contentType)
{
    HttpResponse response;
    HttpHelper::DefaultHttpResponse(&response, HttpVersion::Http_Version_1_1, code, contentType, std::move(content));
    return this_coroutine::WaitAsyncRtnExecute<bool, void>(SendHttpResponse<SocketType>(ctx, this->shared_from_this(), response.EncodePdu()));;
}

template <typename SocketType>
inline AsyncResult<bool, void> HttpStream<SocketType>::SendResponse(RoutineCtx ctx, HttpResponse::uptr response)
{
    return this_coroutine::WaitAsyncRtnExecute<bool, void>(SendHttpResponse<SocketType>(ctx, this->shared_from_this(), response->EncodePdu()));
}

template <typename SocketType>
inline AsyncResult<bool, void> HttpStream<SocketType>::Close()
{
    m_close = true;
    return m_connection->template Close<void>();
}

inline HttpServerConfig::ptr HttpServerConfig::Create()
{
    return std::make_shared<HttpServerConfig>();
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
        HttpStreamConfig config;
        auto server_conf = std::dynamic_pointer_cast<HttpServerConfig>(m_server.GetConfig());
        config.m_max_header_size = server_conf->m_max_header_size;
        config.m_recv_timeout_ms = server_conf->m_recv_timeout_ms;
        auto stream = std::make_shared<HttpStream<SocketType>>(std::move(connection), config, m_map);
        return HttpRouteForward(ctx, stream);
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
inline Coroutine<void> HttpServer<SocketType>::HttpRouteForward(RoutineCtx ctx, typename HttpStream<SocketType>::ptr stream)
{
    return Handle<SocketType>(ctx, stream, m_map);
}


template <typename SocketType>
inline Coroutine<bool> RecvHttpRequestAndHandle(RoutineCtx ctx, typename HttpStream<SocketType>::ptr stream)
{
    stream->m_error.Code() = error::HttpErrorCode::kHttpError_NoError;
    uint32_t max_header_size = stream->GetConfig().m_max_header_size;
    int32_t recv_timeout = static_cast<int64_t>(stream->GetConfig().m_recv_timeout_ms);
    TcpIOVecHolder holder(max_header_size);
    uint32_t offset = 0;
    HttpRequest::uptr request = std::make_unique<HttpRequest>();
    while(offset < max_header_size) {
        int ret = co_await stream->m_connection->template Recv<bool>(holder, max_header_size - offset, recv_timeout);
        if( ret == CommonTcpIORtnType::eCommonDisConnect ) {
            stream->m_error.Code() = error::HttpErrorCode::kHttpError_ConnectionClose;
            co_return false;
        } else if (ret == CommonTcpIORtnType::eCommonTimeOut) {
            stream->m_error.Code() = error::HttpErrorCode::kHttpError_RecvTimeOut;
            std::string response = CodeResponse<HttpStatusCode::RequestTimeout_408>::ResponseStr(HttpVersion::Http_Version_1_1);
            bool res = co_await this_coroutine::WaitAsyncRtnExecute<bool, bool>(SendHttpResponse<SocketType>(ctx, stream, std::move(response)));
            co_return false;
        } else if( ret == CommonTcpIORtnType::eCommonUnknown) {
            stream->m_error.Code() = error::HttpErrorCode::kHttpError_UnkownError;
            std::string response = CodeResponse<HttpStatusCode::BadRequest_400>::ResponseStr(HttpVersion::Http_Version_1_1);
            bool res = co_await this_coroutine::WaitAsyncRtnExecute<bool, bool>(SendHttpResponse<SocketType>(ctx, stream, std::move(response)));
            co_return false;
        } else {
            offset += ret;
        }
        std::string_view view(holder->m_buffer, offset);
        if(!request->ParseHeader(view) ) {
            if(request->GetErrorCode() == error::HttpErrorCode::kHttpError_HeaderInComplete) {
                continue;
            } else {
                std::string response = CodeResponse<HttpStatusCode::BadRequest_400>::ResponseStr(HttpVersion::Http_Version_1_1);
                bool res = co_await this_coroutine::WaitAsyncRtnExecute<bool, bool>(SendHttpResponse<SocketType>(ctx, stream, std::move(response)));
                co_return false;
            }
        } else {
            break;
        }
    }
    holder.ClearBuffer();
    offset = 0;
    uint32_t body_length = 0;
    if( request->Header()->HeaderPairs().HasKey("Content-Length") ) {
        bool right = true;
        try
        {
            body_length = std::stoul(request->Header()->HeaderPairs().GetValue("Content-Length"));
        }
        catch(const std::exception& e)
        {
            right = false;
        }
        if(!right) {
            std::string response = CodeResponse<HttpStatusCode::BadRequest_400>::ResponseStr(HttpVersion::Http_Version_1_1);
            bool res = co_await this_coroutine::WaitAsyncRtnExecute<bool, bool>(SendHttpResponse<SocketType>(ctx, stream, std::move(response)));
            co_return false;
        }
    }
    holder.Realloc(body_length);
    while(offset < body_length) {
        int ret = co_await stream->m_connection->template Recv<bool>(holder, body_length - offset, recv_timeout);
        if( ret == CommonTcpIORtnType::eCommonDisConnect ) {
            stream->m_error.Code() = error::HttpErrorCode::kHttpError_ConnectionClose;
            co_return false;
        } else if (ret == CommonTcpIORtnType::eCommonTimeOut) {
            stream->m_error.Code() = error::HttpErrorCode::kHttpError_RecvTimeOut;
            co_return false;
        } else if( ret == CommonTcpIORtnType::eCommonUnknown) {
            stream->m_error.Code() = error::HttpErrorCode::kHttpError_UnkownError;
            co_return false;
        } else {
            offset += ret;
        }
        if(offset == body_length) {
            std::string_view view = std::string_view(holder->m_buffer, offset);
            request->ParseBody(view);
        }
    }
    stream->GetRequest() = std::move(request);
    co_return true;
}

template <typename SocketType>
inline Coroutine<bool> SendHttpResponse(RoutineCtx ctx, typename HttpStream<SocketType>::ptr stream, std::string &&response)
{
    int length = response.length(), offset = 0;
    TcpIOVecHolder holder(std::move(response));
    bool res = false;
    while(offset < length) {
        int ret = co_await stream->m_connection->template Send<bool>(holder, length - offset, stream->GetConfig().m_recv_timeout_ms);
        if( ret == CommonTcpIORtnType::eCommonDisConnect ) {
            stream->m_error.Code() = error::HttpErrorCode::kHttpError_ConnectionClose;
            co_return std::move(res);
        } else if (ret == CommonTcpIORtnType::eCommonTimeOut) {
            stream->m_error.Code() = error::HttpErrorCode::kHttpError_RecvTimeOut;
            co_return std::move(res);
        } else if( ret == CommonTcpIORtnType::eCommonUnknown) {
            stream->m_error.Code() = error::HttpErrorCode::kHttpError_UnkownError;
            co_return std::move(res);
        } else {
            offset += ret;
        }
    }
    res = true;
    co_return std::move(res);
}


template <typename SocketType>
inline Coroutine<void> Handle(RoutineCtx ctx, typename HttpStream<SocketType>::ptr stream, const typename HttpStream<SocketType>::HandlerMap &handlerMap)
{
    while(!stream->Closed()) {
        bool result = co_await this_coroutine::WaitAsyncRtnExecute<bool, void>(RecvHttpRequestAndHandle<SocketType>(ctx, stream));
        if(!result) {
            bool closed = co_await stream->Close();
            co_return;
        }
        auto& request = stream->GetRequest();
        auto it = handlerMap.find(request->Header()->Method()); 
        if(it == handlerMap.end()) {
            bool res = co_await this_coroutine::WaitAsyncRtnExecute<bool, void>(SendHttpResponse<SocketType>(ctx, stream,\
                CodeResponse<HttpStatusCode::MethodNotAllowed_405>::ResponseStr(HttpVersion::Http_Version_1_1)));
            if(!res) co_return;
        } else {
            auto& uri_map = it->second;
            auto uri_it = uri_map.find(request->Header()->Uri());
            if(uri_it == uri_map.end()) {
                bool res = co_await this_coroutine::WaitAsyncRtnExecute<bool, void>(SendHttpResponse<SocketType>(ctx, stream,\
                    CodeResponse<HttpStatusCode::NotFound_404>::ResponseStr(HttpVersion::Http_Version_1_1)));
                if(!res) co_return;
            } else {
                co_await this_coroutine::WaitAsyncRtnExecute<void>(uri_it->second(ctx, stream));
            }
        }
        
    }
    co_return;
}


}

#endif