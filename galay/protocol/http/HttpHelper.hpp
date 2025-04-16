#ifndef GALAY_HTTP_HELPER_HPP
#define GALAY_HTTP_HELPER_HPP

#include "HttpProtoc.hpp"
#include "galay/utils/System.h"

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

}

#include "HttpHelper.tcc"

#endif