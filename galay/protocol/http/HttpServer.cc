#include "HttpServer.hpp"
#include "galay/kernel/Log.h"
#include "galay/utils/String.h"
#include <regex>

namespace galay::http
{

bool HttpHelper::DefaultGet(HttpRequest *request, const std::string &url, bool keepalive)
{
    std::regex urlPattern("^(https?://)?([^:/]+)(?::(\\d+))?(/.*)?$");
    std::smatch match;

    if (!regex_match(url, match, urlPattern)) {
        return false;
    }
    std::string protocol = match[1].matched? match[1].str() : "http://";
    if( protocol != "http://" || protocol != "https://" ) {
        return false;
    }
    std::string domain = match[2].str();
    int port = match[3].matched? stoi(match[3].str()) : (protocol == "http://"? 80 : 443);
    std::string path = match[4].matched? match[4].str() : "/";
    request->Header()->Version() = HttpVersion::Http_Version_1_1;
    request->Header()->Method() = HttpMethod::Http_Method_Get;
    
    request->Header()->HeaderPairs().AddHeaderPair("Host", domain);
    if(!keepalive) request->Header()->HeaderPairs().AddHeaderPair("Connection", "close");
    else request->Header()->HeaderPairs().AddHeaderPair("Connection", "keep-alive");
    return true;
}

bool HttpHelper::DefaultRedirect(HttpResponse *response, const std::string &url, HttpResponseCode code)
{
    DefaultHttpResponse(response, HttpVersion::Http_Version_1_1, code, "text/html", "");
    response->Header()->HeaderPairs().AddHeaderPair("Location", url);
    return true;
}

bool HttpHelper::DefaultOK(HttpResponse *response, HttpVersion version)
{
    return DefaultHttpResponse(response, version, HttpStatusCode::OK_200, "", "");
}

bool HttpHelper::DefaultHttpResponse(HttpResponse *response, HttpVersion version, HttpStatusCode code, std::string type, std::string &&body)
{
    response->Header()->Version() = version;
    response->Header()->Code() = code;
    response->Header()->HeaderPairs().AddHeaderPair("Server", GALAY_SERVER);
    response->Header()->HeaderPairs().AddHeaderPair("Date", utils::GetCurrentGMTTimeString());
    if(!body.empty()) response->SetContent(type, std::move(body));
    return true;
}


}