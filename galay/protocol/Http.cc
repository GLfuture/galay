#include "Http.h"
#include <spdlog/spdlog.h>

namespace galay::error{

static const char* HttpErrors[] = {
    "No error",
    "Header is incomplete",
    "Body is incomplete",
    "Header is too long",
    "Method is not standard",
    "Uri too long",
    "Chunck has error",
    "Http code is invalid",
    "Header pair exists",
};

bool 
HttpError::HasError() const
{
    return this->m_code != error::kHttpError_NoError;
}

HttpErrorCode& 
HttpError::Code()
{
    return this->m_code;
}

void HttpError::Reset()
{
    this->m_code = kHttpError_NoError;
}

std::string 
HttpError::ToString(HttpErrorCode code) const
{
    return HttpErrors[code];
}


}

namespace galay::protocol::http
{

std::unordered_set<std::string> 
g_stdMethods = {
    "GET", "POST", "PUT", "DELETE", "HEAD", "OPTIONS", "TRACE", "CONNECT" , "PATCH"
};

bool
HeaderPair::HasKey(const std::string& key)
{
    return m_headerPairs.contains(key);
}

std::string 
HeaderPair::GetValue(const std::string& key)
{
    if (m_headerPairs.contains(key))
        return m_headerPairs[key];
    else
        return "";
}

error::HttpErrorCode
HeaderPair::RemoveHeaderPair(const std::string& key)
{
    if (m_headerPairs.contains(key))
    {
        m_headerPairs.erase(key);
    }
    else
    {
        return error::kHttpError_HeaderPairNotExist;
    }
    return error::kHttpError_NoError;
}

error::HttpErrorCode 
HeaderPair::AddHeaderPair(const std::string& key, const std::string& value)
{
    if (m_headerPairs.contains(key))
    {
        return error::kHttpError_HeaderPairExist;
    }
    else
    {
        this->m_headerPairs.emplace(key, value);
    }
    return error::kHttpError_NoError;
}

error::HttpErrorCode 
HeaderPair::SetHeaderPair(const std::string& key, const std::string& value)
{
    if (m_headerPairs.contains(key))
    {
        this->m_headerPairs[key] = value;
    }
    else
    {
        return error::kHttpError_HeaderPairNotExist;
    }
    return error::kHttpError_NoError;
}

std::string 
HeaderPair::ToString()
{
    std::string res;
    for (auto& [k, v] : m_headerPairs)
    {
        res = res + k + ": " + v + "\r\n";
    }
    return std::move(res);
}

void protocol::http::HeaderPair::Clear()
{
    m_headerPairs.clear();
}

void 
HeaderPair::operator=(const HeaderPair& headerPair)
{
    this->m_headerPairs = headerPair.m_headerPairs;
}

std::string& 
HttpRequestHeader::Method()
{
    return this->m_method;
}

std::string& 
HttpRequestHeader::Uri()
{
    return this->m_uri;
}

std::string& 
HttpRequestHeader::Version()
{
    return this->m_version;
}

std::map<std::string,std::string>& 
HttpRequestHeader::Args()
{
    return this->m_argList;
}


HeaderPair& 
HttpRequestHeader::HeaderPairs()
{
    return this->m_headerPairs;
}

error::HttpErrorCode 
HttpRequestHeader::FromString(std::string_view str)
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
                if (!g_stdMethods.contains(m_method))
                {
                    spdlog::error("[{}:{}] [[method is not standard] [Method:{}]]", __FILE__, __LINE__, this->m_method);
                    return error::kHttpError_MethodNotStandard;
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
                    return error::kHttpError_UriTooLong;
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
                    key += str[i];
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
                m_headerPairs.AddHeaderPair(key, value);
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
    return error::kHttpError_NoError;
}

std::string 
HttpRequestHeader::ToString()
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
    res += m_headerPairs.ToString();
    res += "\r\n";
    return std::move(res);
}

void 
HttpRequestHeader::CopyFrom(HttpRequestHeader::ptr header)
{
    this->m_method = header->m_method;
    this->m_uri = header->m_uri;
    this->m_version = header->m_version;
    this->m_argList = header->m_argList;
    this->m_headerPairs = header->m_headerPairs;
}

void protocol::http::HttpRequestHeader::Reset()
{
    m_version.clear();
    m_uri.clear();
    m_method.clear();
    m_argList.clear();
    m_headerPairs.Clear();
}

void 
HttpRequestHeader::ParseArgs(std::string uri)
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
HttpRequestHeader::ConvertFromUri(std::string&& url, bool convert_plus_to_space)
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
HttpRequestHeader::ConvertToUri(std::string&& url)
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
HttpRequestHeader::IsHex(char c, int& v)
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
HttpRequestHeader::ToUtf8(int code, char* buff)
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
HttpRequestHeader::FromHexToI(const std::string& s, size_t i, size_t cnt, int& val)
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
HttpRequest::DecodePdu(const std::string_view& buffer)
{
    m_error->Reset();
    size_t n = buffer.length();
    int eLength = 0;
    //header
    if(m_status == kHttpHeader){
        int pos = buffer.find("\r\n\r\n");
        if(pos == std::string::npos) {
            if (buffer.length() > HTTP_HEADER_MAX_LEN)
            {
                spdlog::error("[{}:{}] [[Header is too long] [Header Len: {} Bytes]]", __FILE__, __LINE__, buffer.length());
                m_error->Code() = error::kHttpError_HeaderTooLong;
                return -1;
            }
            spdlog::debug("[{}:{}] [[Header is incomplete] [Now Rbuffer Len:{} Bytes]]", __FILE__, __LINE__, buffer.length());
            m_error->Code() = error::kHttpError_HeaderInComplete;
            return eLength;
        }
        else if (pos + 4 > HTTP_HEADER_MAX_LEN)
        {
            spdlog::error("[{}:{}] [[Header is too long] [Header Len:{} Bytes]]", __FILE__, __LINE__, pos + 4);
            m_error->Code() = error::kHttpError_HeaderTooLong;
            return -1;
        }
        else{
            spdlog::debug("[{}:{}] [[Header complete] [Header Len:{} Bytes]]", __FILE__, __LINE__, pos + 4);
            if(m_header == nullptr) m_header = std::make_shared<HttpRequestHeader>();
            std::string_view header = buffer;
            error::HttpErrorCode errCode = m_header->FromString(header.substr(0,pos + 4));
            if(errCode != error::kHttpError_NoError){
                m_error->Code() = errCode;
                return -1;
            };
            eLength = pos + 4;
        }
    }
    
    bool hasBody = false;
    //改变状态
    if((m_header->HeaderPairs().HasKey("Transfer-Encoding") && 0 == m_header->HeaderPairs().GetValue("Transfer-Encoding").compare("chunked"))
        || (m_header->HeaderPairs().HasKey("transfer-encoding") && 0 == m_header->HeaderPairs().GetValue("transfer-encoding").compare("chunked")))
    {
        this->m_status = kHttpChunck;
        hasBody = true;
    }else if(m_header->HeaderPairs().HasKey("Content-Length") || m_header->HeaderPairs().HasKey("content-length")){
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
        if(!m_error->HasError()) {
            this->m_status = kHttpHeader;
        }
    }
    
    return eLength;
}

std::string 
HttpRequest::EncodePdu()
{
    if((m_header->HeaderPairs().HasKey("Transfer-Encoding") &&  0 == m_header->HeaderPairs().GetValue("Transfer-Encoding").compare("chunked") )||
        (m_header->HeaderPairs().HasKey("transfer-encoding") && 0 == m_header->HeaderPairs().GetValue("transfer-encoding").compare("chunked"))){
        return m_header->ToString();
    }
    if(!m_header->HeaderPairs().HasKey("Content-Length") && !m_header->HeaderPairs().HasKey("content-length")){
        m_header->HeaderPairs().AddHeaderPair("Content-Length", std::to_string(m_body.length()));
    }
    return m_header->ToString() + m_body;
}

int protocol::http::HttpRequest::GetErrorCode() const
{
    return m_error->Code();
}

std::string protocol::http::HttpRequest::GetErrorString()
{
    return m_error->ToString(m_error->Code());
}

void protocol::http::HttpRequest::Reset()
{
    m_header->Reset();
    m_error->Reset();
    m_body.clear();
    m_status = kHttpHeader;
}

bool protocol::http::HttpRequest::HasError() const
{
    return m_error->HasError();
}

bool 
HttpRequest::StartChunck()
{
    if((m_header->HeaderPairs().HasKey("transfer-encoding") || m_header->HeaderPairs().HasKey("Transfer-Encoding")) && 
        (0 == m_header->HeaderPairs().GetValue("transfer-encoding").compare("chunked") || 0 == m_header->HeaderPairs().GetValue("Transfer-Encoding").compare("chunked"))){
        return true;
    }
    if(m_header->HeaderPairs().HasKey("content-length")){
        m_header->HeaderPairs().RemoveHeaderPair("content-length");
    }
    if(m_header->HeaderPairs().HasKey("Content-Length")){
        m_header->HeaderPairs().RemoveHeaderPair("Content-Length");
    }
    m_header->HeaderPairs().AddHeaderPair("Transfer-Encoding", "chunked");
    return true;
}

std::string 
HttpRequest::ToChunckData(std::string&& buffer)
{
    size_t length = buffer.length();
    std::string res = std::to_string(length);
    res += "\r\n";
    res += buffer;
    res += "\r\n";
    return std::move(res);
}

std::string 
HttpRequest::EndChunck()
{
    return "0\r\n\r\n";
}

HttpRequest::HttpRequest()
{
    this->m_error = std::make_shared<error::HttpError>();
}


HttpRequestHeader::ptr 
HttpRequest::Header()
{
    if(!m_header) m_header = std::make_shared<HttpRequestHeader>();
    return m_header;
}

std::string&
HttpRequest::Body()
{
    return this->m_body;
}

int 
HttpRequest::GetHttpBody(const std::string_view &buffer, int eLength)
{
    size_t n = buffer.length();
    if(m_header->HeaderPairs().HasKey("content-length") || m_header->HeaderPairs().HasKey("Content-Length")){
        std::string contentLength = m_header->HeaderPairs().GetValue("Content-Length");
        if( contentLength.empty() ) contentLength = m_header->HeaderPairs().GetValue("Content-Length");
        size_t length = std::stoul(contentLength);
        if(length <= n) {
            m_body = buffer.substr(eLength, length);
            eLength += length;
            if(buffer.substr(eLength,4).compare("\r\n\r\n") == 0) {
                eLength += 4;
            }  
            spdlog::debug("[{}:{}] [[body is completed] [Body Len:{} Bytes] [Body Package:{}]", __FILE__, __LINE__ , length , this->m_body);
        }else{
            spdlog::warn("[{}:{}] [[body is incomplete] [body len:{} Bytes, expect {} Bytes]]",__FILE__,__LINE__, n, length);
            m_error->Code() = error::kHttpError_BodyInComplete;
            return eLength;
        }
    }else{
        size_t pos = buffer.find("\r\n\r\n", eLength);
        if(pos == std::string::npos){
            if(!buffer.empty()){
                spdlog::warn("[{}:{}] [[body is incomplete] [not end with '\\r\\n\\r\\n']]",__FILE__,__LINE__);
                m_error->Code() = error::kHttpError_BodyInComplete;
                return eLength;
            }
        }else{
            m_body = buffer.substr(eLength, pos - eLength);
            eLength = pos + 4;
        }
    }
    return eLength;
}

int 
HttpRequest::GetChunckBody(const std::string_view& buffer, int eLength)
{
    while (!buffer.empty())
    {
        int pos = buffer.find("\r\n", eLength);
        std::string_view temp = buffer.substr(eLength, pos - eLength);
        int length;
        try
        {
            length = std::stoi(std::string(temp.cbegin(), temp.length()));
        }
        catch (const std::exception &e)
        {
            spdlog::error("[{}:{}] [Chunck is Illegal] [ErrMsg:{}]", __FILE__, __LINE__, e.what());
            m_error->Code() = error::kHttpError_ChunckHasError;
            return eLength;
        }
        if(length == 0){
            eLength = pos + 4;
            spdlog::debug("[{}:{}] [[Chunck is finished] [Chunck Len:{} Bytes]]",__FILE__,__LINE__,pos+4);
            break;
        }else if(length + 4 + pos > buffer.length()){
            spdlog::debug("[{}:{}] [[Chunck is incomplete] [Chunck Len:{} Bytes] [Buffer Len:{} Bytes]]",__FILE__,__LINE__,length + pos + 4,buffer.length());
            m_error->Code() = error::kHttpError_BodyInComplete;
            return eLength;
        }
        this->m_body += buffer.substr(pos+2,length);
        eLength = pos + 4 + length;
    }
    return eLength;
}


std::string& 
HttpResponseHeader::Version()
{
    return this->m_version;
}

int& 
HttpResponseHeader::Code()
{
    return this->m_code;
}

HeaderPair& 
HttpResponseHeader::HeaderPairs()
{
    return this->m_headerPairs;
}

std::string 
HttpResponseHeader::ToString()
{
    std::string res = "HTTP/";
    res = res + this->m_version + ' ' + std::to_string(this->m_code) + ' ' + CodeMsg(this->m_code) + "\r\n";
    res += m_headerPairs.ToString();
    res += "\r\n";
    return res;
}

error::HttpErrorCode 
HttpResponseHeader::FromString(std::string_view str)
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
                    spdlog::error("[{}:{}] [Http status code is illegal]",__FILE__,__LINE__);
                    return error::kHttpError_HttpCodeInvalid;
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
                    key += str[i];
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
                m_headerPairs.AddHeaderPair(key, value);
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
    return error::kHttpError_NoError;
}

std::string 
HttpResponseHeader::CodeMsg(int status)
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

HttpResponse::HttpResponse()
{
    this->m_error = std::make_shared<error::HttpError>();
}


HttpResponseHeader::ptr 
HttpResponse::Header()
{
    if(!m_header) m_header = std::make_shared<HttpResponseHeader>();
    return m_header;
}

std::string& 
HttpResponse::Body()
{
    return m_body;
}

std::string 
HttpResponse::EncodePdu()
{
    if((m_header->HeaderPairs().HasKey("Transfer-Encoding") || m_header->HeaderPairs().HasKey("transfer-encoding")) && 
        ( 0 == m_header->HeaderPairs().GetValue("Transfer-Encoding").compare("chunked") || 0 == m_header->HeaderPairs().GetValue("transfer-encoding").compare("chunked") )){
        return m_header->ToString();
    }
    if(!m_header->HeaderPairs().HasKey("Content-Length") && !m_header->HeaderPairs().HasKey("content-length")){
        m_header->HeaderPairs().AddHeaderPair("Content-Length", std::to_string(m_body.length()));
    }
    return m_header->ToString() + m_body;
}


int 
HttpResponse::DecodePdu(const std::string_view& buffer)
{
    m_error->Reset();
    size_t n = buffer.length();
    int eLength = 0;
    //header
    if(m_status == kHttpHeader){
        int pos = buffer.find("\r\n\r\n");
        if(pos == std::string::npos) {
            if (buffer.length() > HTTP_HEADER_MAX_LEN)
            {
                spdlog::error("[{}:{}] [[Header is too long] [Header Len: {} Bytes]]", __FILE__, __LINE__, buffer.length());
                m_error->Code() = error::kHttpError_HeaderTooLong;
                return -1;
            }
            spdlog::debug("[{}:{}] [[Header is incomplete] [Now Rbuffer Len:{} Bytes]]", __FILE__, __LINE__, buffer.length());
            m_error->Code() = error::kHttpError_HeaderInComplete;
            return eLength;
        }
        else if (pos + 4 > HTTP_HEADER_MAX_LEN)
        {
            spdlog::error("[{}:{}] [[Header is too long] [Header Len:{} Bytes]]", __FILE__, __LINE__, pos + 4);
            m_error->Code() = error::kHttpError_HeaderTooLong;
            return -1;
        }
        else{
            spdlog::debug("[{}:{}] [[Header complete] [Header Len:{} Bytes]]", __FILE__, __LINE__, pos + 4);
            if(m_header == nullptr) m_header = std::make_shared<HttpResponseHeader>();
            std::string_view header = buffer;
            error::HttpErrorCode errCode = m_header->FromString(header.substr(0, pos + 4));
            if(m_error->HasError()){
                m_error->Code() = errCode;
                return -1;
            };
            eLength = pos + 4;
        }
    }
    bool hasBody = false;
    //改变状态
    if((m_header->HeaderPairs().HasKey("Transfer-Encoding") || m_header->HeaderPairs().HasKey("transfer-encoding")) && 
        ( 0 == m_header->HeaderPairs().GetValue("Transfer-Encoding").compare("chunked") || 0 == m_header->HeaderPairs().GetValue("transfer-encoding").compare("chunked"))){
        this->m_status = kHttpChunck;
        hasBody = true;
    }else if(m_header->HeaderPairs().HasKey("content-length") || m_header->HeaderPairs().HasKey("Content-Length")){
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
        if(!m_error->HasError()) {
            this->m_status = kHttpHeader;
        }
    }
    return eLength;
}

bool protocol::http::HttpResponse::HasError() const
{
    return m_error->HasError();
}

int protocol::http::HttpResponse::GetErrorCode() const
{
    return m_error->Code();
}

std::string protocol::http::HttpResponse::GetErrorString()
{
    return m_error->ToString(m_error->Code());
}

void protocol::http::HttpResponse::Reset()
{
    m_error->Reset();
    m_header.reset();
    m_body.clear();
    m_status = kHttpHeader;
}

bool 
HttpResponse::StartChunck()
{
    if(( m_header->HeaderPairs().HasKey("Transfer-Encoding") || m_header->HeaderPairs().HasKey("transfer-encoding")) && 
        ( 0 == m_header->HeaderPairs().GetValue("Transfer-Encoding").compare("chunked") || 0 == m_header->HeaderPairs().GetValue("transfer-encoding").compare("chunked"))){
        return true;
    }
    if(m_header->HeaderPairs().HasKey("content-length")){
        m_header->HeaderPairs().RemoveHeaderPair("content-length");
    }
    if(m_header->HeaderPairs().HasKey("Content-Length")){
        m_header->HeaderPairs().RemoveHeaderPair("Content-Length");
    }
    m_header->HeaderPairs().AddHeaderPair("Transfer-Encoding", "chunked");
    return true;
}

std::string 
HttpResponse::ToChunckData(std::string&& buffer)
{
    size_t length = buffer.length();
    std::string res = std::to_string(length);
    res += "\r\n";
    res += buffer;
    res += "\r\n";
    return std::move(res);
}

std::string 
HttpResponse::EndChunck()
{
    return "0\r\n\r\n";
}


int 
HttpResponse::GetHttpBody(const std::string_view& buffer, int eLength)
{
    size_t n = buffer.length();
    if(m_header->HeaderPairs().HasKey("content-length") || m_header->HeaderPairs().HasKey("Content-Length")){
        std::string contentLength = m_header->HeaderPairs().GetValue("content-length");
        if(contentLength.empty()) contentLength = m_header->HeaderPairs().GetValue("Content-Length");
        size_t length = std::stoul(contentLength);
        if(length <= n) {
            m_body = buffer.substr(eLength, length);
            eLength += length;
            if(buffer.substr(eLength,4).compare("\r\n\r\n") == 0) {
                eLength += 4;
            }  
            spdlog::debug("[{}:{}] [[body is completed] [Body Len:{} Bytes] [Body Package:{}]", __FILE__, __LINE__ , length , this->m_body);
        }else{
            spdlog::warn("[{}:{}] [[body is incomplete] [Body len:{} Bytes, expect {} Bytes]]",__FILE__,__LINE__, n, length);
            m_error->Code() = error::kHttpError_BodyInComplete;
            return eLength;
        }
    }else{
        size_t pos = buffer.find("\r\n\r\n", eLength);
        if(pos == std::string::npos){
            if(!buffer.empty()){
                spdlog::warn("[{}:{}] [[body is incomplete] [not end with '\\r\\n\\r\\n']]",__FILE__,__LINE__);
                m_error->Code() = error::kHttpError_BodyInComplete;
                return eLength;
            }
        }else{
            m_body = buffer.substr(eLength, pos - eLength);
            eLength = pos + 4;
        }
    }
    return eLength;
}


int 
HttpResponse::GetChunckBody(const std::string_view& buffer, int eLength)
{
    while (!buffer.empty())
    {
        int pos = buffer.find("\r\n", eLength);
        std::string_view temp = buffer.substr(eLength, pos - eLength);
        int length;
        try
        {
            length = std::stoi(std::string(temp.cbegin(), temp.length()));
        }
        catch (const std::exception &e)
        {
            spdlog::error("[{}:{}] [Chunck is Illegal] [ErrMsg:{}]", __FILE__, __LINE__, e.what());
            m_error->Code() = error::kHttpError_ChunckHasError;
            return -1;
        }
        if(length == 0){
            eLength = pos + 4;
            spdlog::debug("[{}:{}] [[Chunck is finished] [Chunck Len:{} Bytes]]",__FILE__,__LINE__,pos+4);
            break;
        }else if(length + 4 + pos > buffer.length()){
            spdlog::debug("[{}:{}] [[Chunck is incomplete] [Chunck Len:{} Bytes] [Buffer Len:{} Bytes]]",__FILE__,__LINE__,length + pos + 4,buffer.length());
            m_error->Code() = error::kHttpError_BodyInComplete;
            return 0;
        }
        this->m_body += buffer.substr(pos + 2,length);
        eLength = pos + 4 + length;
    }
    return eLength;
}

//function 

HttpRequest::ptr 
DefaultHttpRequest()
{
    HttpRequest::ptr request = std::make_shared<HttpRequest>();
    HttpRequestHeader::ptr header = request->Header();
    header->Version() = "1.1";
    header->Uri() = "/";
    header->HeaderPairs().AddHeaderPair("Server-Framwork", "galay");
    header->HeaderPairs().AddHeaderPair("Connection", "keep-alive"); 
    return request;
}
}