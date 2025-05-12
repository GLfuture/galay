#ifndef GALAY_HTTP_SERVER_TCC
#define GALAY_HTTP_SERVER_TCC

#include "HttpServer.hpp"

namespace galay::http
{

template <typename SocketType>
inline HttpStreamImpl<SocketType>::HttpStreamImpl(typename Connection<SocketType>::uptr connection, HttpStreamConfig conf)
    : m_connection(std::move(connection)), m_config(conf)
{
}

template <typename SocketType>
inline HttpStreamImpl<SocketType>::HttpError HttpStreamImpl<SocketType>::LastError()
{
    return m_error;
}

template <typename SocketType>
inline HttpStreamConfig &HttpStreamImpl<SocketType>::GetConfig()
{
    return m_config;
}

template <typename SocketType>
inline bool HttpStreamImpl<SocketType>::Closed()
{
    return m_close;
}

template <typename SocketType>
inline AsyncResult<bool, void> HttpStreamImpl<SocketType>::SendResponse(RoutineCtx ctx, HttpStatusCode code, std::string &&content, const std::string &contentType)
{
    HttpResponse response;
    HttpHelper::DefaultHttpResponse(&response, HttpVersion::Http_Version_1_1, code, contentType, std::move(content));
    return this_coroutine::WaitAsyncRtnExecute<bool, void>(SendHttpResponseImpl<SocketType>(ctx, this->shared_from_this(), response.Header()->Code(), response.EncodePdu()));;
}

template <typename SocketType>
inline AsyncResult<bool, void> HttpStreamImpl<SocketType>::SendResponse(RoutineCtx ctx, HttpResponse&& response)
{
    return this_coroutine::WaitAsyncRtnExecute<bool, void>(SendHttpResponseImpl<SocketType>(ctx, this->shared_from_this(), response.Header()->Code(), response.EncodePdu()));
}

template <typename SocketType>
inline AsyncResult<bool, void> HttpStreamImpl<SocketType>::SendFile(RoutineCtx ctx, FileDesc* desc)
{
    return this_coroutine::WaitAsyncRtnExecute<bool, void>(SendStaticFileImpl<SocketType>(ctx, this->shared_from_this(), desc));
}

template <typename SocketType>
inline AsyncResult<bool, void> HttpStreamImpl<SocketType>::Close()
{
    m_close = true;
    return m_connection->template Close<void>();
}

template <typename SocketType>
inline std::pair<std::string, uint16_t> HttpStreamImpl<SocketType>::GetRemoteAddr()
{
    return m_connection->GetRemoteAddr();
}

template <typename SocketType>
inline void HttpStreamImpl<SocketType>::RefreshActiveTime()
{
    m_last_active_time = std::chrono::steady_clock::now();
}

template <typename SocketType>
inline std::chrono::steady_clock::time_point HttpStreamImpl<SocketType>::GetLastActiveTime()
{
    return m_last_active_time;
}

inline HttpServerConfig::ptr HttpServerConfig::Create()
{
    return std::make_shared<HttpServerConfig>();
}


template <typename SocketType>
inline HttpAbstractServer<SocketType>::HttpAbstractServer(HttpServerConfig::ptr config, std::unique_ptr<Logger> logger)
    :m_server(config)
{
    if(logger) HttpLogger::GetInstance()->ResetLogger(std::move(logger));
}

template <typename SocketType>
inline bool HttpAbstractServer<SocketType>::RegisterStaticFileGetMiddleware(const std::string &url_file, const std::string &filesystem_file)
{
    auto middleware = std::make_unique<HttpStaticFileMiddleware>(url_file, filesystem_file);
    return RegisterMiddleware(std::move(middleware));
}

template <typename SocketType>
inline void HttpAbstractServer<SocketType>::Start(THost host)
{
    m_server.OnCall([this](RoutineCtx ctx, Connection<SocketType>::uptr connection) {
        HttpStreamConfig config;
        auto server_conf = std::dynamic_pointer_cast<HttpServerConfig>(m_server.GetConfig());
        config.m_recv_timeout_ms = server_conf->m_recv_timeout_ms;
        config.m_send_timeout_ms = server_conf->m_send_timeout_ms;
        auto stream = std::make_shared<HttpStreamImpl<SocketType>>(std::move(connection), config);
        return HttpRouteForward(ctx, stream);
    });
    m_server.Start(host);
}

template <typename SocketType>
inline void HttpAbstractServer<SocketType>::Stop()
{
    m_server.Stop();
}

template <typename SocketType>
inline bool HttpAbstractServer<SocketType>::IsRunning() const
{
    return m_server.IsRunning();
}

template <typename SocketType>
inline bool HttpAbstractServer<SocketType>::RegisterMiddleware(HttpMiddleware::uptr middleware)
{
    if(!m_middleware) {
        m_middleware = std::move(middleware);
    } else {
        m_middleware->SetNext(std::move(middleware));
    }
    return true;
}

template <typename SocketType>
template <HttpMethod... Methods>
inline void HttpAbstractServer<SocketType>::RouteHandler(const std::string &path, Handler handler)
{
    ([&](){
        this->m_routers[Methods].AddHandler(path, handler);
    }(), ...);
}

template <typename SocketType>
inline Coroutine<void> HttpAbstractServer<SocketType>::HttpRouteForward(RoutineCtx ctx, typename HttpStreamImpl<SocketType>::ptr stream)
{
    return Handle<SocketType>(ctx, stream, m_routers, m_middleware.get());
}


template <typename SocketType>
inline Coroutine<bool> RecvHttpRequestImpl(RoutineCtx ctx, typename HttpStreamImpl<SocketType>::ptr stream, HttpRequest& request)
{
    stream->m_error.Code() = error::HttpErrorCode::kHttpError_NoError;
    int32_t max_header_size = gHttpMaxHeaderSize.load();
    int32_t recv_timeout = static_cast<int64_t>(stream->GetConfig().m_recv_timeout_ms);
    TcpIOVecHolder holder(max_header_size);
    int32_t offset = 0;
    while(offset < max_header_size) {
        int ret = co_await stream->m_connection->template Recv<bool>(holder, max_header_size - offset, recv_timeout);
        if( ret == CommonTcpIORtnType::eCommonDisConnect ) {
            stream->m_error.Code() = error::HttpErrorCode::kHttpError_ConnectionClose;
            co_return false;
        } else if (ret == CommonTcpIORtnType::eCommonTimeOut) {
            stream->m_error.Code() = error::HttpErrorCode::kHttpError_RecvTimeOut;
            std::string response = CodeResponse<HttpStatusCode::RequestTimeout_408>::ResponseStr(HttpVersion::Http_Version_1_1, false);
            bool res = co_await this_coroutine::WaitAsyncRtnExecute<bool, bool>(SendHttpResponseImpl<SocketType>(ctx, stream, HttpStatusCode::RequestTimeout_408, std::move(response)));
            co_return false;
        } else if( ret == CommonTcpIORtnType::eCommonUnknown) {
            stream->m_error.Code() = error::HttpErrorCode::kHttpError_UnkownError;
            std::string response = CodeResponse<HttpStatusCode::BadRequest_400>::ResponseStr(HttpVersion::Http_Version_1_1, false);
            bool res = co_await this_coroutine::WaitAsyncRtnExecute<bool, bool>(SendHttpResponseImpl<SocketType>(ctx, stream, HttpStatusCode::BadRequest_400, std::move(response)));
            co_return false;
        } else {
            offset += ret;
        }
        std::string_view view(holder->m_buffer, offset);
        if(!request.ParseHeader(view) ) {
            if(request.GetErrorCode() == error::HttpErrorCode::kHttpError_HeaderInComplete) {
                continue;
            } else if(request.GetErrorCode() == error::HttpErrorCode::kHttpError_UriTooLong) {
                stream->m_error.Code() = error::HttpErrorCode::kHttpError_UriTooLong;
                std::string response = CodeResponse<HttpStatusCode::UriTooLong_414>::ResponseStr(HttpVersion::Http_Version_1_1, false);
                bool res = co_await this_coroutine::WaitAsyncRtnExecute<bool, bool>(SendHttpResponseImpl<SocketType>(ctx, stream, HttpStatusCode::UriTooLong_414, std::move(response)));
                co_return false;
            } else {
                stream->m_error.Code() = static_cast<error::HttpErrorCode>(request.GetErrorCode());
                std::string response = CodeResponse<HttpStatusCode::BadRequest_400>::ResponseStr(HttpVersion::Http_Version_1_1, false);
                bool res = co_await this_coroutine::WaitAsyncRtnExecute<bool, bool>(SendHttpResponseImpl<SocketType>(ctx, stream, HttpStatusCode::BadRequest_400, std::move(response)));
                co_return false;
            }
        } else {
            break;
        }
    }
    if( offset == max_header_size) {
        stream->m_error.Code() = error::HttpErrorCode::kHttpError_HeaderTooLong;
        std::string response = CodeResponse<HttpStatusCode::RequestHeaderFieldsTooLarge_431>::ResponseStr(HttpVersion::Http_Version_1_1, false);
        bool res = co_await this_coroutine::WaitAsyncRtnExecute<bool, bool>(SendHttpResponseImpl<SocketType>(ctx, stream, HttpStatusCode::RequestHeaderFieldsTooLarge_431, std::move(response)));
        co_return false;
    }
    size_t header_size = request.GetNextIndex();
    offset = request.GetNextIndex();
    uint32_t body_length = 0;
    if( request.Header()->HeaderPairs().HasKey("Content-Length") ) {
        bool right = true;
        try
        {
            body_length = std::stoul(request.Header()->HeaderPairs().GetValue("Content-Length"));
        }
        catch(const std::exception& e)
        {
            right = false;
        }
        if(!right) {
            stream->m_error.Code() = error::HttpErrorCode::kHttpError_BadRequest;
            std::string response = CodeResponse<HttpStatusCode::BadRequest_400>::ResponseStr(HttpVersion::Http_Version_1_1, false);
            bool res = co_await this_coroutine::WaitAsyncRtnExecute<bool, bool>(SendHttpResponseImpl<SocketType>(ctx, stream, HttpStatusCode::BadRequest_400, std::move(response)));
            co_return false;
        }
    }
    size_t expect = header_size + body_length;
    if( holder->m_offset == expect) {
        offset += body_length;
        if(!request.ParseBody(std::string_view(holder->m_buffer, offset))) {
            HttpLogger::GetInstance()->GetLogger()->SpdLogger()->error("ParseBody error, error code: {}", request.GetErrorCode());
            co_return false;
        }
    }
    else {
        holder.Realloc(expect);
    }
    while(offset < expect) {
        int ret = co_await stream->m_connection->template Recv<bool>(holder, expect - offset, recv_timeout);
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
        if(offset == expect) {
            std::string_view view = std::string_view(holder->m_buffer, offset);
            request.ParseBody(view);
        }
    }
    auto remote = stream->GetRemoteAddr();
    SERVER_REQUEST_LOG(request.Header()->Method(), request.Header()->Uri(), remote.first + ":" + std::to_string(remote.second));
    HttpLogger::GetInstance()->GetLogger()->SpdLogger()->debug("{}", std::string_view(holder->m_buffer, offset));
    co_return true;
}

template <typename SocketType>
inline Coroutine<bool> SendHttpResponseImpl(RoutineCtx ctx, typename HttpStreamImpl<SocketType>::ptr stream, HttpStatusCode code , std::string &&response)
{
    std::chrono::time_point end = std::chrono::steady_clock::now();
    SERVER_RESPONSE_LOG(code, std::chrono::duration_cast<std::chrono::milliseconds>(end - stream->m_last_active_time).count());
    int length = response.length(), offset = 0;
    TcpIOVecHolder holder(std::move(response));
    bool res = false;
    while(offset < length) {
        int ret = co_await stream->m_connection->template Send<bool>(holder, length - offset, stream->GetConfig().m_send_timeout_ms);
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
    HttpLogger::GetInstance()->GetLogger()->SpdLogger()->debug("{}", std::string_view(holder->m_buffer, offset));
    res = true;
    co_return std::move(res);
}

template <typename SocketType>
Coroutine<bool> SendStaticFileImpl(RoutineCtx ctx, typename HttpStreamImpl<SocketType>::ptr stream, FileDesc *desc)
{
    bool res = false;
    while(desc->m_offset < desc->m_file_size) {
        int ret = co_await stream->m_connection->template SendFile<bool>(desc, stream->GetConfig().m_send_timeout_ms);
        if( ret == CommonTcpIORtnType::eCommonDisConnect ) {
            stream->m_error.Code() = error::HttpErrorCode::kHttpError_ConnectionClose;
            co_return std::move(res);
        } else if (ret == CommonTcpIORtnType::eCommonTimeOut) {
            stream->m_error.Code() = error::HttpErrorCode::kHttpError_RecvTimeOut;
            co_return std::move(res);
        } else if( ret == CommonTcpIORtnType::eCommonUnknown) {
            stream->m_error.Code() = error::HttpErrorCode::kHttpError_UnkownError;
            co_return std::move(res);
        }
    }
    res = true;
    co_return std::move(res);
}

template <typename SocketType>
inline Coroutine<void> Handle(RoutineCtx ctx, typename HttpStreamImpl<SocketType>::ptr stream, std::unordered_map<HttpMethod, HttpRouter>& routers, HttpMiddleware* middleware)
{
    std::any user_data;
    while(!stream->Closed()) {
        HttpContext context(HttpRequest(), stream, user_data);
        auto& request = context.GetRequest();
        stream->RefreshActiveTime();
        bool result = co_await this_coroutine::WaitAsyncRtnExecute<bool, void>(RecvHttpRequestImpl<SocketType>(ctx, stream, request));
        stream->RefreshActiveTime();
        if(!result) {
            bool closed = co_await stream->Close();
            co_return;
        }
        bool close = (request.Header()->HeaderPairs().GetValue("Connection").compare("close") == 0 || request.Header()->HeaderPairs().GetValue("connection").compare("close") == 0);
        if( middleware ){
            bool res = co_await this_coroutine::WaitAsyncRtnExecute<bool, void>(middleware->HandleRequest(ctx, context));
            if(context.GetStream()->Closed()) {
                bool closed = co_await stream->Close();
                co_return;
            }
            if(!res) {
                continue;
            }
        }
        auto it = routers.find(request.Header()->Method()); 
        if(it == routers.end()) {
            bool res = co_await this_coroutine::WaitAsyncRtnExecute<bool, void>(SendHttpResponseImpl<SocketType>(ctx, stream, HttpStatusCode::MethodNotAllowed_405,\
                CodeResponse<HttpStatusCode::MethodNotAllowed_405>::ResponseStr(HttpVersion::Http_Version_1_1, false)));
            if(!res) {
                continue;
            }
        } else {
            auto& router = it->second;
            HttpRouter::Handler handler;
            std::unordered_map<std::string, std::string> params;
            if(!router.Find(request.Header()->Uri(), handler, params)) {
                bool res = co_await this_coroutine::WaitAsyncRtnExecute<bool, void>(SendHttpResponseImpl<SocketType>(ctx, stream, HttpStatusCode::NotFound_404,\
                    CodeResponse<HttpStatusCode::NotFound_404>::ResponseStr(HttpVersion::Http_Version_1_1, false)));
                if(!res) {
                    bool closed = co_await stream->Close();
                    co_return;
                }
            } else {
                context.AddParam(std::move(params));
                co_await this_coroutine::WaitAsyncRtnExecute<void>(handler(ctx, std::move(context)));
            }
        }
        if(close) {
            bool closed = co_await stream->Close();
            co_return;
        }
    }
    co_return;
}
}

#endif