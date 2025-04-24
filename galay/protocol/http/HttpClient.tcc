#ifndef GALAY_HTTP_CLIENT_TCC
#define GALAY_HTTP_CLIENT_TCC

#include "HttpClient.hpp"


namespace galay::http
{

template <typename SocketType>
Coroutine<bool> SendHttpRequestImpl(RoutineCtx ctx, HttpAbstractClient<SocketType> *client, HttpRequest &&request, int64_t timeout)
{
    THost host = client->GetRemoteAddr();
    auto remote = host.m_ip + ":" + std::to_string(host.m_port);
    auto request_str = request.EncodePdu();
    int length = request_str.length(), offset = 0;
    TcpIOVecHolder holder(std::move(request_str));
    while(offset < length) {
        int ret = co_await client->m_client.template Send<bool>(holder, length - offset, timeout);
        if( ret == CommonTcpIORtnType::eCommonDisConnect ) {
            client->m_error.Code() = error::HttpErrorCode::kHttpError_ConnectionClose;
            co_return false;
        } else if (ret == CommonTcpIORtnType::eCommonTimeOut) {
            client->m_error.Code() = error::HttpErrorCode::kHttpError_RecvTimeOut;
            co_return false;
        } else if( ret == CommonTcpIORtnType::eCommonUnknown) {
            client->m_error.Code() = error::HttpErrorCode::kHttpError_UnkownError;
            co_return false;
        } else {
            offset += ret;
        }
    }
    CLIENT_REQUEST_LOG(request.Header()->Method(), request.Header()->Uri(), remote);
    co_return true;
}

template <typename SocketType>
Coroutine<bool> RecvHttpResponseImpl(RoutineCtx ctx, HttpAbstractClient<SocketType> *client, HttpResponse* response, int64_t timeout)
{
    client->m_error.Code() = error::HttpErrorCode::kHttpError_NoError;
    int32_t max_header_size = client->m_max_header_size;
    int32_t recv_timeout = timeout;
    TcpIOVecHolder holder(max_header_size);
    int32_t offset = 0;
    while(offset < max_header_size) {
        int ret = co_await client->m_client.template Recv<bool>(holder, max_header_size - offset, recv_timeout);
        if( ret == CommonTcpIORtnType::eCommonDisConnect ) {
            client->m_error.Code() = error::HttpErrorCode::kHttpError_ConnectionClose;
            co_return false;
        } else if (ret == CommonTcpIORtnType::eCommonTimeOut) {
            client->m_error.Code() = error::HttpErrorCode::kHttpError_RecvTimeOut;
            co_return false;
        } else if( ret == CommonTcpIORtnType::eCommonUnknown) {
            client->m_error.Code() = error::HttpErrorCode::kHttpError_UnkownError;
            co_return false;
        } else {
            offset += ret;
        }
        std::string_view view(holder->m_buffer, offset);
        if(!response->ParseHeader(view) ) {
            if(response->GetErrorCode() == error::HttpErrorCode::kHttpError_HeaderInComplete) {
                continue;
            }  else {
                client->m_error.Code() = static_cast<error::HttpErrorCode>(response->GetErrorCode());
                co_return false;
            }
        } else {
            break;
        }
    }
    if( offset == max_header_size) {
        client->m_error.Code() = error::HttpErrorCode::kHttpError_HeaderTooLong;
        co_return false;
    }
    size_t header_size = response->GetNextIndex();
    offset = response->GetNextIndex();
    uint32_t body_length = 0;
    if( response->Header()->HeaderPairs().HasKey("Content-Length") ) {
        bool right = true;
        try
        {
            body_length = std::stoul(response->Header()->HeaderPairs().GetValue("Content-Length"));
        }
        catch(const std::exception& e)
        {
            right = false;
        }
        if(!right) {
            client->m_error.Code() = error::HttpErrorCode::kHttpError_BadRequest;
            co_return false;
        }
    }
    size_t expect = header_size + body_length;
    if( holder->m_offset == expect) {
        offset += body_length;
        response->ParseBody(std::string_view(holder->m_buffer, offset));
    }
    else {
        holder.Realloc(expect);
    }
    while(offset < expect) {
        int ret = co_await client->m_client.template Recv<bool>(holder, expect - offset, recv_timeout);
        if( ret == CommonTcpIORtnType::eCommonDisConnect ) {
            client->m_error.Code() = error::HttpErrorCode::kHttpError_ConnectionClose;
            co_return false;
        } else if (ret == CommonTcpIORtnType::eCommonTimeOut) {
            client->m_error.Code() = error::HttpErrorCode::kHttpError_RecvTimeOut;
            co_return false;
        } else if( ret == CommonTcpIORtnType::eCommonUnknown) {
            client->m_error.Code() = error::HttpErrorCode::kHttpError_UnkownError;
            co_return false;
        } else {
            offset += ret;
        }
        if(offset == body_length) {
            std::string_view view = std::string_view(holder->m_buffer, offset);
            response->ParseBody(view);
        }
    }
    CLIENT_RESPONSE_LOG(response->Header()->Code(), std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - client->m_last_time).count());
    co_return true;
}


template <typename SocketType>
Coroutine<HttpResponse> Handle(RoutineCtx ctx, HttpAbstractClient<SocketType> *client, HttpRequest &&request, int64_t timeout)
{
    HttpResponse response;
    bool res = co_await this_coroutine::WaitAsyncRtnExecute<bool, HttpResponse>(SendHttpRequestImpl<SocketType>(ctx, client, std::move(request), timeout));
    if(!res) {
        co_return std::move(response);
    }
    client->m_last_time = std::chrono::steady_clock::now();
    res = co_await this_coroutine::WaitAsyncRtnExecute<bool, HttpResponse>(RecvHttpResponseImpl<SocketType>(ctx, client, &response, timeout));
    co_return std::move(response);
}


template <typename SocketType>
inline HttpAbstractClient<SocketType>::HttpAbstractClient(GHandle handle)
    :m_client(handle)
{
}

template <typename SocketType>
inline std::string HttpAbstractClient<SocketType>::GetHttpErrorString()
{
    return m_error.ToString(GetErrorCode());
}

template <typename SocketType>
inline uint32_t HttpAbstractClient<SocketType>::GetErrorCode()
{
    uint32_t code = m_client.GetErrorCode();
    if(code != 0) {
        return code;
    }
    code = m_error.Code();
    return code;
}

template <typename SocketType>
inline THost HttpAbstractClient<SocketType>::GetRemoteAddr()
{
    return m_client.GetRemoteAddr();
}

template <typename SocketType>
template <typename CoRtn>
inline AsyncResult<bool, CoRtn> HttpAbstractClient<SocketType>::Connect(THost *addr, int64_t timeout)
{
    return m_client.template Connect<CoRtn>(addr, timeout);
}


template <typename SocketType>
template <typename CoRtn>
inline AsyncResult<bool, CoRtn> HttpAbstractClient<SocketType>::Connect(const std::string &url, int64_t timeout)
{
    std::regex pattern(R"(^http://($$([a-fA-F0-9:]+)$$|([a-zA-Z0-9\-\.]+))(?::(\d+))?$)");
    std::smatch matches;
    if (!std::regex_match(url, matches, pattern)) {
        m_error.Code() = error::HttpErrorCode::kHttpError_UrlInvalid;
        return {false};
    }
    std::string host;
    if (!matches[2].str().empty()) {  // IPv6
        host = matches[2].str();
    } else {                          // IPv4/域名
        host = matches[3].str();
    }

    uint16_t port = 80;
    if (!matches[4].str().empty()) {
        try {
            int parsed_port = std::stoi(matches[4].str());
            if (parsed_port < 1 || parsed_port > 65535) {
                m_error.Code() = error::HttpErrorCode::kHttpError_PortInvalid;
                return {false};
            }
            port = static_cast<uint16_t>(parsed_port);
        } catch (const std::exception&) {
            m_error.Code() = error::HttpErrorCode::kHttpError_PortInvalid;
            return {false};
        }
    }
    THost addr;
    addr.m_ip = host;
    addr.m_port = port;
    return Connect(&addr, timeout);
}

template <typename SocketType>
template <typename CoRtn>
inline AsyncResult<HttpResponse, CoRtn> HttpAbstractClient<SocketType>::Get(RoutineCtx ctx, const std::string &url, int64_t timeout, bool keepalive)
{
    HttpRequest request;
    HttpHelper::DefaultGet(&request, url , keepalive);
    return this_coroutine::WaitAsyncRtnExecute<HttpResponse, CoRtn>(Handle<SocketType>(ctx, this, std::move(request), timeout));
}

template <typename SocketType>
template <typename CoRtn>
inline AsyncResult<HttpResponse, CoRtn> HttpAbstractClient<SocketType>::Post(RoutineCtx ctx, const std::string &url, std::string &&content, std::string&& content_type, int64_t timeout, bool keepalive)
{
    HttpRequest request;
    HttpHelper::DefaultPost(&request, url, std::move(content), std::move(content_type), keepalive);
    return this_coroutine::WaitAsyncRtnExecute<HttpResponse, CoRtn>(Handle<SocketType>(ctx, this, std::move(request), timeout));
}


template <typename SocketType>
template <typename CoRtn>
inline AsyncResult<HttpResponse, CoRtn> HttpAbstractClient<SocketType>::Put(RoutineCtx ctx, const std::string &url, std::string &&content, std::string&& content_type, int64_t timeout, bool keepalive)
{
    HttpRequest request;
    HttpHelper::DefaultPut(&request, url, std::move(content), std::move(content_type), keepalive);
    return this_coroutine::WaitAsyncRtnExecute<HttpResponse, CoRtn>(Handle<SocketType>(ctx, this, std::move(request), timeout));
}

template <typename SocketType>
template <typename CoRtn>
inline AsyncResult<HttpResponse, CoRtn> HttpAbstractClient<SocketType>::Delete(RoutineCtx ctx, const std::string &url, std::string &&content, std::string &&content_type, int64_t timeout, bool keepalive)
{
    HttpRequest request;
    HttpHelper::DefaultDelete(&request, url, std::move(content), std::move(content_type), keepalive);
    return this_coroutine::WaitAsyncRtnExecute<HttpResponse, CoRtn>(Handle<SocketType>(ctx, this, std::move(request), timeout));
}

template <typename SocketType>
template <typename CoRtn>
inline AsyncResult<bool, CoRtn> HttpAbstractClient<SocketType>::Close()
{
    return m_client->template Close<CoRtn>();
}
}

#endif