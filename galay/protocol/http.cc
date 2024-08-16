#include "http.h"
#include <spdlog/spdlog.h>


std::unordered_set<std::string> 
galay::protocol::http::m_stdMethods = {
    "GET", "POST", "PUT", "DELETE", "HEAD", "OPTIONS", "TRACE", "CONNECT" , "PATCH"
};


std::string& 
galay::protocol::http::HttpRequestHeader::Method()
{
    return this->m_method;
}

std::string& 
galay::protocol::http::HttpRequestHeader::Uri()
{
    return this->m_uri;
}

std::string& 
galay::protocol::http::HttpRequestHeader::Version()
{
    return this->m_version;
}

std::map<std::string,std::string>& 
galay::protocol::http::HttpRequestHeader::Args()
{
    return this->m_argList;
}


std::map<std::string,std::string>& 
galay::protocol::http::HttpRequestHeader::Headers()
{
    return this->m_headers;
}

bool 
galay::protocol::http::HttpRequestHeader::FromString(std::string_view str)
{
    HttpHeadStatus status = HttpHeadStatus::kHttpHeadMethod;
    std::string key, value;
    size_t n = str.size();
    size_t i;
    for (i = 0; i < n; ++i)
    {
        if(status == HttpHeadStatus::kHttpHeadEnd) break;
        switch (status)
        {
        case kHttpHeadMethod:
        {
            if (str[i] != ' ')
            {
                m_method += str[i];
            }
            else
            {
                if (!m_stdMethods.contains(m_method))
                {
                    spdlog::error("[{}:{}] [[method is not standard] [Method:{}]]", __FILE__, __LINE__, this->m_method);
                    return false;
                }
                status = HttpHeadStatus::kHttpHeadUri;
            }
        }
        break;
        case kHttpHeadUri:
        {
            if (str[i] != ' ')
            {
                m_uri += str[i];
            }
            else
            {
                if (m_uri.length() > HTTP_URI_MAX_LEN)
                {
                    spdlog::error("[{}:{}] [[uri is too long] [Uri:{}]]", __FILE__, __LINE__, this->m_uri);
                    return false;
                }
                this->m_uri = ConvertFromUri(std::move(this->m_uri), false);
                ParseArgs(this->m_uri);
                status = HttpHeadStatus::kHttpHeadVersion;
            }
        }
        break;
        case kHttpHeadVersion:
        {
            if (str[i] != '\r')
            {
                m_version += str[i];
            }
            else
            {
                if (m_version.substr(0, 5) != "HTTP/")
                    return false;
                m_version = m_version.substr(m_version.find('/') + 1);
                status = HttpHeadStatus::kHttpHeadKey;
                ++i;
            }
        }
        break;
        case kHttpHeadKey:
        {
            if (str[i] == '\r')
            {
                ++i;
                status = HttpHeadStatus::kHttpHeadEnd;
            }
            else
            {
                if (str[i] != ':')
                {
                    key += std::tolower(str[i]);
                }
                else
                {
                    if (i + 1 < n && str[i + 1] == ' ')
                        ++i;
                    status = HttpHeadStatus::kHttpHeadValue;
                }
            }
        }
        break;
        case kHttpHeadValue:
        {
            if (str[i] != '\r')
            {
                value += str[i];
            }
            else
            {
                m_headers[key] = value;
                key.clear();
                value.clear();
                ++i;
                status = HttpHeadStatus::kHttpHeadKey;
            }
        }
        break;
        default:
            break;
        }
    }
    return true;
}

std::string 
galay::protocol::http::HttpRequestHeader::ToString()
{
    std::string res = this->m_method + " ";
    std::string url = this->m_uri;
    if (!m_argList.empty())
    {
        url += '?';
        for (auto& [k, v] : m_argList)
        {
            url = url + k + '=' + v + '&';
        }
        url.erase(--url.end());
    }
    res += ConvertToUri(std::move(url));
    res = res + " HTTP/" + this->m_version + "\r\n";
    for (auto& [k, v] : m_headers)
    {
        res = res + k + ": " + v + "\r\n";
    }
    res += "\r\n";
    return std::move(res);
}

void 
galay::protocol::http::HttpRequestHeader::CopyFrom(HttpRequestHeader::ptr header)
{
    this->m_method = header->m_method;
    this->m_uri = header->m_uri;
    this->m_version = header->m_version;
    this->m_argList = header->m_argList;
    this->m_headers = header->m_headers;
}

void 
galay::protocol::http::HttpRequestHeader::ParseArgs(std::string uri)
{
    int argindx = uri.find('?');
    if (argindx != std::string::npos)
    {
        int cur = 0;
        this->m_uri = uri.substr(cur, argindx - cur);
        std::string args = uri.substr(argindx + 1);
        std::string key = "", value = "";
        int status = 0;
        for (int i = 0; i < args.length(); i++)
        {
            if (status == 0)
            {
                if (args[i] == '=')
                {
                    status = 1;
                }
                else
                {
                    key += args[i];
                }
            }
            else
            {
                if (args[i] == '&')
                {
                    this->m_argList[key] = value;
                    key = "", value = "";
                    status = 0;
                }
                else
                {
                    value += args[i];
                    if (i == args.length() - 1)
                    {
                        this->m_argList[key] = value;
                    }
                }
            }
        }
    }
}

std::string 
galay::protocol::http::HttpRequestHeader::ConvertFromUri(std::string&& url, bool convert_plus_to_space)
{
    std::string result;
    for (size_t i = 0; i < url.size(); i++)
    {
        if (url[i] == '%' && i + 1 < url.size())
        {
            if (url[i + 1] == 'u')
            {
                auto val = 0;
                if (FromHexToI(url, i + 2, 4, val))
                {
                    char buff[4];
                    size_t len = ToUtf8(val, buff);
                    if (len > 0)
                    {
                        result.append(buff, len);
                    }
                    i += 5;
                }
                else
                {
                    result += url[i];
                }
            }
            else
            {
                auto val = 0;
                if (FromHexToI(url, i + 1, 2, val))
                {
                    result += static_cast<char>(val);
                    i += 2;
                }
                else
                {
                    result += url[i];
                }
            }
        }
        else if (convert_plus_to_space && url[i] == '+')
        {
            result += ' ';
        }
        else
        {
            result += url[i];
        }
    }

    return result;
}

std::string 
galay::protocol::http::HttpRequestHeader::ConvertToUri(std::string&& url)
{
    std::string result;
    size_t n = url.size();
    for (size_t i = 0; i < n ; i++)
    {
        switch (url[i])
        {
        case ' ':
            result += "%20";
            break;
        case '+':
            result += "%2B";
            break;
        case '\r':
            result += "%0D";
            break;
        case '\n':
            result += "%0A";
            break;
        case '\'':
            result += "%27";
            break;
        case ',':
            result += "%2C";
            break;
            // case ':': result += "%3A"; break; // ok? probably...
        case ';':
            result += "%3B";
            break;
        default:
            auto c = static_cast<uint8_t>(url[i]);
            if (c >= 0x80)
            {
                result += '%';
                char hex[4];
                auto len = snprintf(hex, sizeof(hex) - 1, "%02X", c);
                assert(len == 2);
                result.append(hex, static_cast<size_t>(len));
            }
            else
            {
                result += url[i];
            }
            break;
        }
    }

    return std::move(result);

}

bool 
galay::protocol::http::HttpRequestHeader::IsHex(char c, int& v)
{
    if (0x20 <= c && isdigit(c))
    {
        v = c - '0';
        return true;
    }
    else if ('A' <= c && c <= 'F')
    {
        v = c - 'A' + 10;
        return true;
    }
    else if ('a' <= c && c <= 'f')
    {
        v = c - 'a' + 10;
        return true;
    }
    return false;
}

size_t 
galay::protocol::http::HttpRequestHeader::ToUtf8(int code, char* buff)
{
    if (code < 0x0080)
    {
        buff[0] = static_cast<char>(code & 0x7F);
        return 1;
    }
    else if (code < 0x0800)
    {
        buff[0] = static_cast<char>(0xC0 | ((code >> 6) & 0x1F));
        buff[1] = static_cast<char>(0x80 | (code & 0x3F));
        return 2;
    }
    else if (code < 0xD800)
    {
        buff[0] = static_cast<char>(0xE0 | ((code >> 12) & 0xF));
        buff[1] = static_cast<char>(0x80 | ((code >> 6) & 0x3F));
        buff[2] = static_cast<char>(0x80 | (code & 0x3F));
        return 3;
    }
    else if (code < 0xE000)
    {
        return 0;
    }
    else if (code < 0x10000)
    {
        buff[0] = static_cast<char>(0xE0 | ((code >> 12) & 0xF));
        buff[1] = static_cast<char>(0x80 | ((code >> 6) & 0x3F));
        buff[2] = static_cast<char>(0x80 | (code & 0x3F));
        return 3;
    }
    else if (code < 0x110000)
    {
        buff[0] = static_cast<char>(0xF0 | ((code >> 18) & 0x7));
        buff[1] = static_cast<char>(0x80 | ((code >> 12) & 0x3F));
        buff[2] = static_cast<char>(0x80 | ((code >> 6) & 0x3F));
        buff[3] = static_cast<char>(0x80 | (code & 0x3F));
        return 4;
    }
    return 0;
}

bool 
galay::protocol::http::HttpRequestHeader::FromHexToI(const std::string& s, size_t i, size_t cnt, int& val)
{
    if (i >= s.size())
    {
        return false;
    }

    val = 0;
    for (; cnt; i++, cnt--)
    {
        if (!s[i])
        {
            return false;
        }
        auto v = 0;
        if (IsHex(s[i], v))
        {
            val = val * 16 + v;
        }
        else
        {
            return false;
        }
    }
    return true;
}


int 
galay::protocol::http::HttpRequest::DecodePdu(const std::string& buffer)
{
    Success();
    size_t n = buffer.length();
    int eLength = 0;
    //header
    if(m_status == kHttpHeader){
        int pos = buffer.find("\r\n\r\n");
        if(pos == std::string::npos) {
            if (buffer.length() > HTTP_HEADER_MAX_LEN)
            {
                spdlog::error("[{}:{}] [[Header is too long] [Header Len: {} Bytes]]", __FILE__, __LINE__, buffer.length());
                Illegal();
                return -1;
            }
            spdlog::debug("[{}:{}] [[Header is incomplete] [Now Rbuffer Len:{} Bytes]]", __FILE__, __LINE__, buffer.length());
            Incomplete();
            return eLength;
        }
        else if (pos + 4 > HTTP_HEADER_MAX_LEN)
        {
            spdlog::error("[{}:{}] [[Header is too long] [Header Len:{} Bytes]]", __FILE__, __LINE__, pos + 4);
            Illegal();
            return -1;
        }
        else{
            spdlog::debug("[{}:{}] [[Header complete] [Header Len:{} Bytes]]", __FILE__, __LINE__, pos + 4);
            if(m_header == nullptr) m_header = std::make_shared<HttpRequestHeader>();
            std::string_view header = buffer;
            if(!m_header->FromString(header.substr(0,pos))){
                Illegal();
                return -1;
            };
            eLength = pos + 4;
        }
    }
    
    bool hasBody = false;
    //改变状态
    if(m_header->Headers().contains("transfer-encoding") && 0 == m_header->Headers()["transfer-encoding"].compare("chunked")){
        this->m_status = kHttpChunck;
        hasBody = true;
    }else if(m_header->Headers().contains("Content-Length")){
        this->m_status = kHttpBody;
        hasBody = true;
    }
    
    //hasBody
    if(hasBody) 
    {
        //根据状态处理
        switch (this->m_status)
        {
        case kHttpBody:
        {
            eLength = GetHttpBody(buffer, eLength);
            break;
        }
        case kHttpChunck:
        {
            eLength = GetChunckBody(buffer, eLength);
            break;
        }
        default:
            break;
        }
        if(ParseSuccess()) {
            this->m_status = kHttpHeader;
        }
    }
    
    return eLength;
}

std::string 
galay::protocol::http::HttpRequest::EncodePdu()
{
    if((m_header->Headers().contains("transfer-encoding") || m_header->Headers().contains("Transfer-Encoding")) && 
        (0 == m_header->Headers()["transfer-encoding"].compare("chunked") || 0 == m_header->Headers()["Transfer-Encoding"].compare("chunked"))){
        return m_header->ToString();
    }
    if(!m_header->Headers().contains("Content-Length") && !m_header->Headers().contains("content-length")){
        m_header->Headers()["Content-Length"] = std::to_string(m_body.length());
    }
    return m_header->ToString() + m_body + "\r\n\r\n";
}

bool 
galay::protocol::http::HttpRequest::StartChunck()
{
    if((m_header->Headers().contains("transfer-encoding") || m_header->Headers().contains("Transfer-Encoding")) && 
        (0 == m_header->Headers()["transfer-encoding"].compare("chunked") || 0 == m_header->Headers()["Transfer-Encoding"].compare("chunked"))){
        return true;
    }
    if(m_header->Headers().contains("content-length")){
        m_header->Headers().erase("content-length");
    }
    if(m_header->Headers().contains("Content-Length")){
        m_header->Headers().erase("Content-Length");
    }
    m_header->Headers()["Transfer-Encoding"] = "chunked";
    return true;
}

std::string 
galay::protocol::http::HttpRequest::ChunckStream(std::string&& buffer)
{
    size_t length = buffer.length();
    std::string res = std::to_string(length);
    res += "\r\n";
    res += buffer;
    res += "\r\n";
    return std::move(res);
}

std::string 
galay::protocol::http::HttpRequest::EndChunck()
{
    return "0\r\n\r\n";
}

galay::protocol::http::HttpRequestHeader::ptr 
galay::protocol::http::HttpRequest::Header()
{
    if(!m_header) m_header = std::make_shared<HttpRequestHeader>();
    return m_header;
}

std::string&
galay::protocol::http::HttpRequest::Body()
{
    return this->m_body;
}

int 
galay::protocol::http::HttpRequest::GetHttpBody(const std::string &buffer, int eLength)
{
    size_t n = buffer.length();
    if(m_header->Headers().contains("content-length")){
        size_t length = std::stoul(m_header->Headers()["content-length"]);
        if(length <= n) {
            m_body = buffer.substr(eLength, length);
            eLength += length;
            if(buffer.substr(eLength,4).compare("\r\n\r\n") == 0) {
                eLength += 4;
            }  
            spdlog::debug("[{}:{}] [[body is completed] [Body Len:{} Bytes] [Body Package:{}]", __FILE__, __LINE__ , length , this->m_body);
        }else{
            spdlog::warn("[{}:{}] [[body is incomplete] [body len:{} Bytes, expect {} Bytes]]",__FILE__,__LINE__, n, length);
            Incomplete();
        }
    }else{
        size_t pos = buffer.find("\r\n\r\n", eLength);
        if(pos == std::string::npos){
            if(!buffer.empty()){
                spdlog::warn("[{}:{}] [[body is incomplete] [not end with '\\r\\n\\r\\n']]",__FILE__,__LINE__);
                Incomplete();
            }
        }else{
            m_body = buffer.substr(eLength, pos - eLength);
            eLength = pos + 4;
        }
    }
    Success();
    return eLength;
}

int 
galay::protocol::http::HttpRequest::GetChunckBody(const std::string& buffer, int eLength)
{
    while (!buffer.empty())
    {
        int pos = buffer.find("\r\n", eLength);
        std::string temp = buffer.substr(eLength, pos - eLength);
        int length;
        try
        {
            length = std::stoi(temp);
        }
        catch (const std::exception &e)
        {
            spdlog::error("[{}:{}] [Chunck is Illegal] [ErrMsg:{}]", __FILE__, __LINE__, e.what());
            Illegal();
            return eLength;
        }
        if(length == 0){
            eLength = pos + 4;
            spdlog::debug("[{}:{}] [[Chunck is finished] [Chunck Len:{} Bytes]]",__FILE__,__LINE__,pos+4);
            break;
        }else if(length + 4 + pos > buffer.length()){
            spdlog::debug("[{}:{}] [[Chunck is incomplete] [Chunck Len:{} Bytes] [Buffer Len:{} Bytes]]",__FILE__,__LINE__,length + pos + 4,buffer.length());
            Incomplete();
            return 0;
        }
        this->m_body += buffer.substr(pos+2,length);
        eLength = pos + 4 + length;
    }
    Success();
    return eLength;
}

std::string& 
galay::protocol::http::HttpResponseHeader::Version()
{
    return this->m_version;
}

int& 
galay::protocol::http::HttpResponseHeader::Code()
{
    return this->m_code;
}

std::map<std::string, std::string>& 
galay::protocol::http::HttpResponseHeader::Headers()
{
    return this->m_headers;
}

std::string 
galay::protocol::http::HttpResponseHeader::ToString()
{
    std::string res = "HTTP/";
    res = res + this->m_version + ' ' + std::to_string(this->m_code) + ' ' + CodeMsg(this->m_code) + "\r\n";
    for (auto& [k, v] : this->m_headers)
    {
        res = res + k + ": " + v + "\r\n";
    }
    res += "\r\n";
    return res;
}

bool 
galay::protocol::http::HttpResponseHeader::FromString(std::string_view str)
{
    size_t n = str.length();
    HttpHeadStatus status = HttpHeadStatus::kHttpHeadVersion;
    std::string status_code;
    std::string key,value;
    size_t i;
    for (i = 0; i < n; ++i)
    {
        if (status == HttpHeadStatus::kHttpHeadEnd)
            break;
        switch (status)
        {
        case kHttpHeadVersion:
        {
            if (str[i] != ' ')
            {
                m_version += str[i];
            }
            else
            {
                if (m_version.substr(0, 5) != "HTTP/"){
                    spdlog::error("[{}:{}] [Http version is illegal]",__FILE__,__LINE__);
                    return false;
                }
                m_version = m_version.substr(m_version.find('/') + 1);
                status = HttpHeadStatus::kHttpHeadStatusCode;
            }
        }
        break;
        case kHttpHeadStatusCode:
        {
            if (str[i] != ' ')
            {
                status_code += str[i];
            }
            else
            {
                try
                {
                    m_code = std::stoi(status_code);
                }
                catch (std::invalid_argument &e)
                {
                    spdlog::error("[{}:{}] [error: 'http status code is illegal',buffer len:{}]",__FILE__,__LINE__,n);
                    return false;
                }
                status = HttpHeadStatus::kHttpHeadStatusMsg;
            }
        }
        break;
        case kHttpHeadStatusMsg:
        {
            if (str[i] == '\r')
            {
                status = HttpHeadStatus::kHttpHeadKey;
                ++i;
            }
        }
        break;
        case kHttpHeadKey:
        {
            if (str[i] == '\r')
            {
                ++i;
                status = HttpHeadStatus::kHttpHeadEnd;
            }
            else
            {
                if (str[i] != ':')
                {
                    key += std::tolower(str[i]);
                }
                else
                {
                    if (i + 1 < n && str[i + 1] == ' ')
                        ++i;
                    status = HttpHeadStatus::kHttpHeadValue;
                }
            }
        }
        break;
        case kHttpHeadValue:
        {
            if (str[i] != '\r')
            {
                value += str[i];
            }
            else
            {
                m_headers[key] = value;
                key.clear();
                value.clear();
                ++i;
                status = HttpHeadStatus::kHttpHeadKey;
            }
        }
        break;
        default:
            break;
        }
    }
    return true;
}

std::string 
galay::protocol::http::HttpResponseHeader::CodeMsg(int status)
{
    switch (status)
    {
    case HttpResponseStatus::Continue_100:
        return "Continue";
    case HttpResponseStatus::SwitchingProtocol_101:
        return "Switching Protocol";
    case HttpResponseStatus::Processing_102:
        return "Processing";
    case HttpResponseStatus::EarlyHints_103:
        return "Early Hints";
    case HttpResponseStatus::OK_200:
        return "OK";
    case HttpResponseStatus::Created_201:
        return "Created";
    case HttpResponseStatus::Accepted_202:
        return "Accepted";
    case HttpResponseStatus::NonAuthoritativeInformation_203:
        return "Non-Authoritative Information";
    case HttpResponseStatus::NoContent_204:
        return "No Content";
    case HttpResponseStatus::ResetContent_205:
        return "Reset Content";
    case HttpResponseStatus::PartialContent_206:
        return "Partial Content";
    case HttpResponseStatus::MultiStatus_207:
        return "Multi-Status";
    case HttpResponseStatus::AlreadyReported_208:
        return "Already Reported";
    case HttpResponseStatus::IMUsed_226:
        return "IM Used";
    case HttpResponseStatus::MultipleChoices_300:
        return "Multiple Choices";
    case HttpResponseStatus::MovedPermanently_301:
        return "Moved Permanently";
    case HttpResponseStatus::Found_302:
        return "Found";
    case HttpResponseStatus::SeeOther_303:
        return "See Other";
    case HttpResponseStatus::NotModified_304:
        return "Not Modified";
    case HttpResponseStatus::UseProxy_305:
        return "Use Proxy";
    case HttpResponseStatus::Unused_306:
        return "unused";
    case HttpResponseStatus::TemporaryRedirect_307:
        return "Temporary Redirect";
    case HttpResponseStatus::PermanentRedirect_308:
        return "Permanent Redirect";
    case HttpResponseStatus::BadRequest_400:
        return "Bad Request";
    case HttpResponseStatus::Unauthorized_401:
        return "Unauthorized";
    case HttpResponseStatus::PaymentRequired_402:
        return "Payment Required";
    case HttpResponseStatus::Forbidden_403:
        return "Forbidden";
    case HttpResponseStatus::NotFound_404:
        return "Not Found";
    case HttpResponseStatus::MethodNotAllowed_405:
        return "Method Not Allowed";
    case HttpResponseStatus::NotAcceptable_406:
        return "Not Acceptable";
    case HttpResponseStatus::ProxyAuthenticationRequired_407:
        return "Proxy Authentication Required";
    case HttpResponseStatus::RequestTimeout_408:
        return "Request Timeout";
    case HttpResponseStatus::Conflict_409:
        return "Conflict";
    case HttpResponseStatus::Gone_410:
        return "Gone";
    case HttpResponseStatus::LengthRequired_411:
        return "Length Required";
    case HttpResponseStatus::PreconditionFailed_412:
        return "Precondition Failed";
    case HttpResponseStatus::PayloadTooLarge_413:
        return "Payload Too Large";
    case HttpResponseStatus::UriTooLong_414:
        return "URI Too Long";
    case HttpResponseStatus::UnsupportedMediaType_415:
        return "Unsupported Media Type";
    case HttpResponseStatus::RangeNotSatisfiable_416:
        return "Range Not Satisfiable";
    case HttpResponseStatus::ExpectationFailed_417:
        return "Expectation Failed";
    case HttpResponseStatus::ImATeapot_418:
        return "I'm a teapot";
    case HttpResponseStatus::MisdirectedRequest_421:
        return "Misdirected Request";
    case HttpResponseStatus::UnprocessableContent_422:
        return "Unprocessable Content";
    case HttpResponseStatus::Locked_423:
        return "Locked";
    case HttpResponseStatus::FailedDependency_424:
        return "Failed Dependency";
    case HttpResponseStatus::TooEarly_425:
        return "Too Early";
    case HttpResponseStatus::UpgradeRequired_426:
        return "Upgrade Required";
    case HttpResponseStatus::PreconditionRequired_428:
        return "Precondition Required";
    case HttpResponseStatus::TooManyRequests_429:
        return "Too Many Requests";
    case HttpResponseStatus::RequestHeaderFieldsTooLarge_431:
        return "Request Header Fields Too Large";
    case HttpResponseStatus::UnavailableForLegalReasons_451:
        return "Unavailable For Legal Reasons";
    case HttpResponseStatus::NotImplemented_501:
        return "Not Implemented";
    case HttpResponseStatus::BadGateway_502:
        return "Bad Gateway";
    case HttpResponseStatus::ServiceUnavailable_503:
        return "Service Unavailable";
    case HttpResponseStatus::GatewayTimeout_504:
        return "Gateway Timeout";
    case HttpResponseStatus::HttpVersionNotSupported_505:
        return "HTTP Version Not Supported";
    case HttpResponseStatus::VariantAlsoNegotiates_506:
        return "Variant Also Negotiates";
    case HttpResponseStatus::InsufficientStorage_507:
        return "Insufficient Storage";
    case HttpResponseStatus::LoopDetected_508:
        return "Loop Detected";
    case HttpResponseStatus::NotExtended_510:
        return "Not Extended";
    case HttpResponseStatus::NetworkAuthenticationRequired_511:
        return "Network Authentication Required";
    default:
    case HttpResponseStatus::InternalServerError_500:
        return "Internal Server Error";
    }
    return "";
}

galay::protocol::http::HttpResponseHeader::ptr 
galay::protocol::http::HttpResponse::Header()
{
    if(!m_header) m_header = std::make_shared<HttpResponseHeader>();
    return m_header;
}

std::string& 
galay::protocol::http::HttpResponse::Body()
{
    return m_body;
}

std::string 
galay::protocol::http::HttpResponse::EncodePdu()
{
    if((m_header->Headers().contains("transfer-encoding") || m_header->Headers().contains("Transfer-Encoding")) && 
        (0 == m_header->Headers()["transfer-encoding"].compare("chunked") || 0 == m_header->Headers()["Transfer-Encoding"].compare("chunked"))){
        return m_header->ToString();
    }
    if(!m_header->Headers().contains("Content-Length") && !m_header->Headers().contains("content-length")){
        m_header->Headers()["Content-Length"] = std::to_string(m_body.length());
    }
    return m_header->ToString() + m_body + "\r\n\r\n";
}


int 
galay::protocol::http::HttpResponse::DecodePdu(const std::string& buffer)
{
    Success();
    size_t n = buffer.length();
    int eLength = 0;
    //header
    if(m_status == kHttpHeader){
        int pos = buffer.find("\r\n\r\n");
        if(pos == std::string::npos) {
            if (buffer.length() > HTTP_HEADER_MAX_LEN)
            {
                spdlog::error("[{}:{}] [[Header is too long] [Header Len: {} Bytes]]", __FILE__, __LINE__, buffer.length());
                Illegal();
                return -1;
            }
            spdlog::debug("[{}:{}] [[Header is incomplete] [Now Rbuffer Len:{} Bytes]]", __FILE__, __LINE__, buffer.length());
            Incomplete();
            return eLength;
        }
        else if (pos + 4 > HTTP_HEADER_MAX_LEN)
        {
            spdlog::error("[{}:{}] [[Header is too long] [Header Len:{} Bytes]]", __FILE__, __LINE__, pos + 4);
            Illegal();
            return -1;
        }
        else{
            spdlog::debug("[{}:{}] [[Header complete] [Header Len:{} Bytes]]", __FILE__, __LINE__, pos + 4);
            if(m_header == nullptr) m_header = std::make_shared<HttpResponseHeader>();
            std::string_view header = buffer;
            if(!m_header->FromString(header.substr(0,pos))){
                Illegal();
                return -1;
            };
            eLength = pos + 4;
        }
    }
    bool hasBody = false;
    //改变状态
    if(m_header->Headers().contains("transfer-encoding") && 0 == m_header->Headers()["transfer-encoding"].compare("chunked")){
        this->m_status = kHttpChunck;
        hasBody = true;
    }else if(m_header->Headers().contains("content-length")){
        this->m_status = kHttpBody;
        hasBody = true;
    }

    if(hasBody) 
    {
        //根据状态处理
        switch (this->m_status)
        {
        case kHttpBody:
        {
            eLength = GetHttpBody(buffer, eLength);
            break;
        }
        case kHttpChunck:
        {
            eLength = GetChunckBody(buffer, eLength);
            break;
        }
        default:
            break;
        }
        if(ParseSuccess()) {
            this->m_status = kHttpHeader;
        }
    }
    return eLength;
}

bool 
galay::protocol::http::HttpResponse::StartChunck()
{
    if((m_header->Headers().contains("transfer-encoding") || m_header->Headers().contains("Transfer-Encoding")) && 
        (0 == m_header->Headers()["transfer-encoding"].compare("chunked") || 0 == m_header->Headers()["Transfer-Encoding"].compare("chunked"))){
        return true;
    }
    if(m_header->Headers().contains("content-length")){
        m_header->Headers().erase("content-length");
    }
    if(m_header->Headers().contains("Content-Length")){
        m_header->Headers().erase("Content-Length");
    }
    m_header->Headers()["Transfer-Encoding"] = "chunked";
    return true;
}

std::string 
galay::protocol::http::HttpResponse::ChunckStream(std::string&& buffer)
{
    size_t length = buffer.length();
    std::string res = std::to_string(length);
    res += "\r\n";
    res += buffer;
    res += "\r\n";
    return std::move(res);
}

std::string 
galay::protocol::http::HttpResponse::EndChunck()
{
    return "0\r\n\r\n";
}


int 
galay::protocol::http::HttpResponse::GetHttpBody(const std::string& buffer, int eLength)
{
    size_t n = buffer.length();
    if(m_header->Headers().contains("content-length")){
        size_t length = std::stoul(m_header->Headers()["content-length"]);
        if(length <= n) {
            m_body = buffer.substr(eLength, length);
            eLength += length;
            if(buffer.substr(eLength,4).compare("\r\n\r\n") == 0) {
                eLength += 4;
            }  
            spdlog::debug("[{}:{}] [[body is completed] [Body Len:{} Bytes] [Body Package:{}]", __FILE__, __LINE__ , length , this->m_body);
        }else{
            spdlog::warn("[{}:{}] [[body is incomplete] [Body len:{} Bytes, expect {} Bytes]]",__FILE__,__LINE__, n, length);
            Incomplete();
            return eLength;
        }
    }else{
        size_t pos = buffer.find("\r\n\r\n", eLength);
        if(pos == std::string::npos){
            if(!buffer.empty()){
                spdlog::warn("[{}:{}] [[body is incomplete] [not end with '\\r\\n\\r\\n']]",__FILE__,__LINE__);
                Incomplete();
                return eLength;
            }
        }else{
            m_body = buffer.substr(eLength, pos - eLength);
            eLength = pos + 4;
        }
    }
    Success();
    return eLength;
}


int 
galay::protocol::http::HttpResponse::GetChunckBody(const std::string& buffer, int eLength)
{
    while (!buffer.empty())
    {
        int pos = buffer.find("\r\n", eLength);
        std::string temp = buffer.substr(eLength, pos - eLength);
        int length;
        try
        {
            length = std::stoi(temp);
        }
        catch (const std::exception &e)
        {
            spdlog::error("[{}:{}] [Chunck is Illegal] [ErrMsg:{}]", __FILE__, __LINE__, e.what());
            Illegal();
            return -1;
        }
        if(length == 0){
            eLength = pos + 4;
            spdlog::debug("[{}:{}] [[Chunck is finished] [Chunck Len:{} Bytes]]",__FILE__,__LINE__,pos+4);
            break;
        }else if(length + 4 + pos > buffer.length()){
            spdlog::debug("[{}:{}] [[Chunck is incomplete] [Chunck Len:{} Bytes] [Buffer Len:{} Bytes]]",__FILE__,__LINE__,length + pos + 4,buffer.length());
            Incomplete();
            return 0;
        }
        this->m_body += buffer.substr(pos + 2,length);
        eLength = pos + 4 + length;
    }
    Success();
    return eLength;
}


//function 

galay::protocol::http::HttpRequest::ptr 
galay::protocol::http::DefaultHttpRequest()
{
    HttpRequest::ptr request = std::make_shared<HttpRequest>();
    HttpRequestHeader::ptr header = request->Header();
    header->Version() = "1.1";
    header->Uri() = "/";
    header->Headers()["Server"] = "Galay-Server";
    header->Headers()["Connection"] = "keep-alive"; 
    return request;
}