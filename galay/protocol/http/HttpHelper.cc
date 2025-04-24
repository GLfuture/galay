#include "HttpHelper.hpp"
#include "galay/utils/String.h"

#include <regex>

namespace galay::http 
{
bool HttpHelper::DefaultGet(HttpRequest *request, const std::string &url, bool keepalive) 
{
    std::regex urlPattern(R"(^(https?)://([^:/?#]+)(?::(\d+))?(/[^?#]*)?$)");
    std::smatch match;

    if (!std::regex_match(url, match, urlPattern)) {
        return false;
    }

    // 协议处理（强制小写）
    std::string protocol = match[1].str();
    std::transform(protocol.begin(), protocol.end(), protocol.begin(), ::tolower);
    if (protocol != "http" && protocol != "https") {
        return false;
    }

    // 主机处理
    std::string domain = match[2].str();

    // 端口处理
    int default_port = (protocol == "http") ? 80 : 443;
    int port = default_port;
    if (match[3].matched) {
        try {
            port = std::stoi(match[3].str());
            if (port < 1 || port > 65535) return false;
        } catch (...) {
            return false;
        }
    }

    // 路径处理（带默认值）
    std::string path = match[4].matched ? match[4].str() : "/";

    // 构造请求头（带端口的主机头）
    request->Header()->Version() = HttpVersion::Http_Version_1_1;
    request->Header()->Method() = HttpMethod::Http_Method_Get;
    request->Header()->Uri() = path;
    
    // 包含端口的主机头（当端口非默认时）
    std::string host_header = domain;
    if (port != default_port) {
        host_header += ":" + std::to_string(port);
    }
    request->Header()->HeaderPairs().AddHeaderPair("Host", host_header);

    request->Header()->HeaderPairs().AddHeaderPair("Connection", 
        keepalive ? "keep-alive" : "close");
    
    return true;
}

bool HttpHelper::DefaultPost(HttpRequest *request, const std::string &url, std::string&& content, std::string&& content_type, bool keepalive)
{
    std::regex urlPattern(R"(^(https?)://([^:/?#]+)(?::(\d+))?(/[^?#]*)?$)");
    std::smatch match;

    if (!std::regex_match(url, match, urlPattern)) {
        return false;
    }

    // 协议处理（强制小写）
    std::string protocol = match[1].str();
    std::transform(protocol.begin(), protocol.end(), protocol.begin(), ::tolower);
    if (protocol != "http" && protocol != "https") {
        return false;
    }

    // 主机处理
    std::string domain = match[2].str();

    // 端口处理
    int default_port = (protocol == "http") ? 80 : 443;
    int port = default_port;
    if (match[3].matched) {
        try {
            port = std::stoi(match[3].str());
            if (port < 1 || port > 65535) return false;
        } catch (...) {
            return false;
        }
    }

    // 路径处理（带默认值）
    std::string path = match[4].matched ? match[4].str() : "/";

    // 构造请求头（带端口的主机头）
    request->Header()->Version() = HttpVersion::Http_Version_1_1;
    request->Header()->Method() = HttpMethod::Http_Method_Post;
    request->Header()->Uri() = path;
    
    // 包含端口的主机头（当端口非默认时）
    std::string host_header = domain;
    if (port != default_port) {
        host_header += ":" + std::to_string(port);
    }
    request->Header()->HeaderPairs().AddHeaderPair("Host", host_header);

    request->Header()->HeaderPairs().AddHeaderPair("Connection", 
        keepalive ? "keep-alive" : "close");
    request->SetContent(std::move(content), std::move(content_type));
    return true;
}

bool HttpHelper::DefaultPut(HttpRequest *request, const std::string &url, std::string &&content, std::string &&content_type, bool keepalive)
{
    std::regex urlPattern(R"(^(https?)://([^:/?#]+)(?::(\d+))?(/[^?#]*)?$)");
    std::smatch match;

    if (!std::regex_match(url, match, urlPattern)) {
        return false;
    }

    // 协议处理（强制小写）
    std::string protocol = match[1].str();
    std::transform(protocol.begin(), protocol.end(), protocol.begin(), ::tolower);
    if (protocol != "http" && protocol != "https") {
        return false;
    }

    // 主机处理
    std::string domain = match[2].str();

    // 端口处理
    int default_port = (protocol == "http") ? 80 : 443;
    int port = default_port;
    if (match[3].matched) {
        try {
            port = std::stoi(match[3].str());
            if (port < 1 || port > 65535) return false;
        } catch (...) {
            return false;
        }
    }

    // 路径处理（带默认值）
    std::string path = match[4].matched ? match[4].str() : "/";

    // 构造请求头（带端口的主机头）
    request->Header()->Version() = HttpVersion::Http_Version_1_1;
    request->Header()->Method() = HttpMethod::Http_Method_Put;
    request->Header()->Uri() = path;
    
    // 包含端口的主机头（当端口非默认时）
    std::string host_header = domain;
    if (port != default_port) {
        host_header += ":" + std::to_string(port);
    }
    request->Header()->HeaderPairs().AddHeaderPair("Host", host_header);

    request->Header()->HeaderPairs().AddHeaderPair("Connection", 
        keepalive ? "keep-alive" : "close");
    if(!content.empty()) request->SetContent(std::move(content), std::move(content_type));
    return true;
}

bool HttpHelper::DefaultDelete(HttpRequest *request, const std::string &url, std::string &&content, std::string &&content_type, bool keepalive)
{
    std::regex urlPattern(R"(^(https?)://([^:/?#]+)(?::(\d+))?(/[^?#]*)?$)");
    std::smatch match;

    if (!std::regex_match(url, match, urlPattern)) {
        return false;
    }

    // 协议处理（强制小写）
    std::string protocol = match[1].str();
    std::transform(protocol.begin(), protocol.end(), protocol.begin(), ::tolower);
    if (protocol != "http" && protocol != "https") {
        return false;
    }

    // 主机处理
    std::string domain = match[2].str();

    // 端口处理
    int default_port = (protocol == "http") ? 80 : 443;
    int port = default_port;
    if (match[3].matched) {
        try {
            port = std::stoi(match[3].str());
            if (port < 1 || port > 65535) return false;
        } catch (...) {
            return false;
        }
    }

    // 路径处理（带默认值）
    std::string path = match[4].matched ? match[4].str() : "/";

    // 构造请求头（带端口的主机头）
    request->Header()->Version() = HttpVersion::Http_Version_1_1;
    request->Header()->Method() = HttpMethod::Http_Method_Put;
    request->Header()->Uri() = path;
    
    // 包含端口的主机头（当端口非默认时）
    std::string host_header = domain;
    if (port != default_port) {
        host_header += ":" + std::to_string(port);
    }
    request->Header()->HeaderPairs().AddHeaderPair("Host", host_header);

    request->Header()->HeaderPairs().AddHeaderPair("Connection", 
        keepalive ? "keep-alive" : "close");
    if(!content.empty()) request->SetContent(std::move(content), std::move(content_type));
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