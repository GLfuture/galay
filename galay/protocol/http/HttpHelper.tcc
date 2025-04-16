#ifndef GALAY_HTTP_HELPER_TCC
#define GALAY_HTTP_HELPER_TCC

#include "HttpHelper.hpp"

namespace galay::http 
{


template <HttpStatusCode Code>
inline std::string CodeResponse<Code>::ResponseStr(HttpVersion version, bool keepAlive)
{
    if(m_responseStr.empty()) {
        m_responseStr = DefaultResponse(version, keepAlive);
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
inline std::string CodeResponse<Code>::DefaultResponse(HttpVersion version, bool keepAlive)
{
    HttpResponse response;
    response.Header()->Code() = Code;
    response.Header()->Version() = version;
    response.Header()->HeaderPairs().AddHeaderPair("Content-Type", "text/html");
    response.Header()->HeaderPairs().AddHeaderPair("Server", GALAY_SERVER);
    response.Header()->HeaderPairs().AddHeaderPair("Date", utils::GetCurrentGMTTimeString());
    if(!keepAlive) response.Header()->HeaderPairs().AddHeaderPair("Connection", "close");
    else response.Header()->HeaderPairs().AddHeaderPair("Connection", "keep-alive");
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
    

}


#endif

