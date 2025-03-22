#include "Http.hpp"
#include "galay/kernel/Log.h"
#include "galay/utils/String.h"
#include <regex>

namespace galay::error{

static const char* HttpErrors[] = {
    "No error",
    "Header is incomplete",
    "Body is incomplete",
    "Header is too long",
    "Uri too long",
    "Chunck has error",
    "Http code is invalid",
    "Header pair exists",
    "Header pair not exist",
    "Bad Request",
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
HttpError::ToString(const HttpErrorCode code) const
{
    return HttpErrors[code];
}


}

namespace galay::http
{

std::vector<std::string> 
g_HttpMethods = {
    "GET", "POST", "HEAD", "PUT", "DELETE", "TRACE", "OPTIONS", "CONNECT" , "PATCH", "UNKNOWN"
};

std::vector<std::string> 
g_HttpVersion = {
    "HTTP/1.0",
    "HTTP/1.1",
    "HTTP/2.0",
    "HTTP/3.0",
    "Unknown"
};

std::string HttpVersionToString(HttpVersion version)
{
    return g_HttpVersion[static_cast<int>(version)];
}

HttpVersion StringToHttpVersion(std::string_view str)
{
    for (int i = 0; i < g_HttpVersion.size(); ++i) {
        if (str == g_HttpVersion[i]) {
            return static_cast<HttpVersion>(i);
        }
    }
    return HttpVersion::Http_Version_Unknown;
}

std::string HttpMethodToString(HttpMethod method)
{
    return g_HttpMethods[static_cast<int>(method)];
}

HttpMethod StringToHttpMethod(std::string_view str)
{
    for (int i = 0; i < g_HttpMethods.size(); ++i) {
        if (str == g_HttpMethods[i]) {
            return static_cast<HttpMethod>(i);
        }
    }
    return HttpMethod::Http_Method_Unknown;
}

std::string HttpStatusCodeToString(HttpStatusCode code)
{
    switch (code)
    {
    case HttpStatusCode::Continue_100:
        return "Continue";
    case HttpStatusCode::SwitchingProtocol_101:
        return "Switching Protocol";
    case HttpStatusCode::Processing_102:
        return "Processing";
    case HttpStatusCode::EarlyHints_103:
        return "Early Hints";
    case HttpStatusCode::OK_200:
        return "OK";
    case HttpStatusCode::Created_201:
        return "Created";
    case HttpStatusCode::Accepted_202:
        return "Accepted";
    case HttpStatusCode::NonAuthoritativeInformation_203:
        return "Non-Authoritative Information";
    case HttpStatusCode::NoContent_204:
        return "No Content";
    case HttpStatusCode::ResetContent_205:
        return "Reset Content";
    case HttpStatusCode::PartialContent_206:
        return "Partial Content";
    case HttpStatusCode::MultiStatus_207:
        return "Multi-Status";
    case HttpStatusCode::AlreadyReported_208:
        return "Already Reported";
    case HttpStatusCode::IMUsed_226:
        return "IM Used";
    case HttpStatusCode::MultipleChoices_300:
        return "Multiple Choices";
    case HttpStatusCode::MovedPermanently_301:
        return "Moved Permanently";
    case HttpStatusCode::Found_302:
        return "Found";
    case HttpStatusCode::SeeOther_303:
        return "See Other";
    case HttpStatusCode::NotModified_304:
        return "Not Modified";
    case HttpStatusCode::UseProxy_305:
        return "Use Proxy";
    case HttpStatusCode::Unused_306:
        return "unused";
    case HttpStatusCode::TemporaryRedirect_307:
        return "Temporary Redirect";
    case HttpStatusCode::PermanentRedirect_308:
        return "Permanent Redirect";
    case HttpStatusCode::BadRequest_400:
        return "Bad Request";
    case HttpStatusCode::Unauthorized_401:
        return "Unauthorized";
    case HttpStatusCode::PaymentRequired_402:
        return "Payment Required";
    case HttpStatusCode::Forbidden_403:
        return "Forbidden";
    case HttpStatusCode::NotFound_404:
        return "Not Found";
    case HttpStatusCode::MethodNotAllowed_405:
        return "Method Not Allowed";
    case HttpStatusCode::NotAcceptable_406:
        return "Not Acceptable";
    case HttpStatusCode::ProxyAuthenticationRequired_407:
        return "Proxy Authentication Required";
    case HttpStatusCode::RequestTimeout_408:
        return "Request Timeout";
    case HttpStatusCode::Conflict_409:
        return "Conflict";
    case HttpStatusCode::Gone_410:
        return "Gone";
    case HttpStatusCode::LengthRequired_411:
        return "Length Required";
    case HttpStatusCode::PreconditionFailed_412:
        return "Precondition Failed";
    case HttpStatusCode::PayloadTooLarge_413:
        return "Payload Too Large";
    case HttpStatusCode::UriTooLong_414:
        return "URI Too Long";
    case HttpStatusCode::UnsupportedMediaType_415:
        return "Unsupported Media Type";
    case HttpStatusCode::RangeNotSatisfiable_416:
        return "Range Not Satisfiable";
    case HttpStatusCode::ExpectationFailed_417:
        return "Expectation Failed";
    case HttpStatusCode::ImATeapot_418:
        return "I'm a teapot";
    case HttpStatusCode::MisdirectedRequest_421:
        return "Misdirected Request";
    case HttpStatusCode::UnprocessableContent_422:
        return "Unprocessable Content";
    case HttpStatusCode::Locked_423:
        return "Locked";
    case HttpStatusCode::FailedDependency_424:
        return "Failed Dependency";
    case HttpStatusCode::TooEarly_425:
        return "Too Early";
    case HttpStatusCode::UpgradeRequired_426:
        return "Upgrade Required";
    case HttpStatusCode::PreconditionRequired_428:
        return "Precondition Required";
    case HttpStatusCode::TooManyRequests_429:
        return "Too Many Requests";
    case HttpStatusCode::RequestHeaderFieldsTooLarge_431:
        return "Request Header Fields Too Large";
    case HttpStatusCode::UnavailableForLegalReasons_451:
        return "Unavailable For Legal Reasons";
    case HttpStatusCode::NotImplemented_501:
        return "Not Implemented";
    case HttpStatusCode::BadGateway_502:
        return "Bad Gateway";
    case HttpStatusCode::ServiceUnavailable_503:
        return "Service Unavailable";
    case HttpStatusCode::GatewayTimeout_504:
        return "Gateway Timeout";
    case HttpStatusCode::HttpVersionNotSupported_505:
        return "HTTP Version Not Supported";
    case HttpStatusCode::VariantAlsoNegotiates_506:
        return "Variant Also Negotiates";
    case HttpStatusCode::InsufficientStorage_507:
        return "Insufficient Storage";
    case HttpStatusCode::LoopDetected_508:
        return "Loop Detected";
    case HttpStatusCode::NotExtended_510:
        return "Not Extended";
    case HttpStatusCode::NetworkAuthenticationRequired_511:
        return "Network Authentication Required";
    default:
    case HttpStatusCode::InternalServerError_500:
        return "Internal Server Error";
    }
    return "";
}

std::unordered_map<std::string, std::string> MimeType::mimeTypeMap = {
    {"html", "text/html"},
    {"htm", "text/html"},
    {"shtml", "text/html"},
    {"css", "text/css"},
    {"xml", "text/xml"},
    {"gif", "image/gif"},
    {"jpeg", "image/jpeg"},
    {"jpg", "image/jpeg"},
    {"js", "application/javascript"},
    {"atom", "application/atom+xml"},
    {"rss", "application/rss+xml"},
    {"mml", "text/mathml"},
    {"txt", "text/plain"},
    {"jad", "text/vnd.sun.j2me.app-descriptor"},
    {"wml", "text/vnd.wap.wml"},
    {"htc", "text/x-component"},
    {"png", "image/png"},
    {"tif", "image/tiff"},
    {"tiff", "image/tiff"},
    {"wbmp", "image/vnd.wap.wbmp"},
    {"ico", "image/x-icon"},
    {"jpe", "image/jpeg"},
    {"json", "application/json"},
    {"pdf", "application/pdf"},
    {"zip", "application/zip"},
    {"tar", "application/x-tar"},
    {"gz", "application/gzip"},
    {"bz2", "application/x-bzip2"},
    {"mp3", "audio/mpeg"},
    {"wav", "audio/wav"},
    {"mp4", "video/mp4"},
    {"avi", "video/x-msvideo"},
    {"mov", "video/quicktime"},
    {"flv", "video/x-flv"},
    {"swf", "application/x-shockwave-flash"},
    {"doc", "application/msword"},
    {"docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
    {"xls", "application/vnd.ms-excel"},
    {"xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
    {"ppt", "application/vnd.ms-powerpoint"},
    {"pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
    {"rtf", "application/rtf"},
    {"ps", "application/postscript"},
    {"eps", "application/postscript"},
    {"svg", "image/svg+xml"},
    {"svgz", "image/svg+xml"},
    {"csv", "text/csv"},
    {"tsv", "text/tab-separated-values"},
    {"md", "text/markdown"},
    {"markdown", "text/markdown"},
    {"log", "text/plain"},
    {"conf", "text/plain"},
    {"ini", "text/plain"},
    {"sh", "application/x-sh"},
    {"bat", "application/x-msdos-program"},
    {"py", "text/x-python"},
    {"pl", "application/x-perl"},
    {"php", "application/x-httpd-php"},
    {"rb", "application/x-ruby"},
    {"java", "text/x-java-source"},
    {"c", "text/x-c"},
    {"cpp", "text/x-c++"},
    {"h", "text/x-c"},
    {"hpp", "text/x-c++"},
    {"cs", "text/x-csharp"},
    {"go", "text/x-go"},
    {"swift", "text/x-swift"},
    {"kt", "text/x-kotlin"},
    {"rs", "text/x-rust"},
    {"dart", "text/x-dart"},
    {"ts", "text/typescript"},
    {"tsx", "text/typescript-jsx"},
    {"jsx", "text/jsx"},
    {"vue", "text/x-vue"},
    {"less", "text/x-less"},
    {"scss", "text/x-scss"},
    {"sass", "text/x-sass"},
    {"styl", "text/x-stylus"},
    {"map", "application/json"},
    {"map.json", "application/json"},
    {"woff", "font/woff"},
    {"woff2", "font/woff2"},
    {"ttf", "font/ttf"},
    {"otf", "font/otf"},
    {"eot", "application/vnd.ms-fontobject"},
    {"oga", "audio/ogg"},
    {"ogv", "video/ogg"},
    {"ogx", "application/ogg"},
    {"webm", "video/webm"},
    {"aac", "audio/aac"},
    {"oga", "audio/ogg"},
    {"opus", "audio/opus"},
    {"webp", "image/webp"},
    {"bmp", "image/bmp"},
    {"weba", "audio/webm"},
    {"mid", "audio/midi"},
    {"midi", "audio/midi"},
    {"mpga", "audio/mpeg"},
    {"mp2", "audio/mpeg"},
    {"mp2a", "audio/mpeg"},
    {"m3u", "audio/x-mpegurl"},
    {"m3u8", "application/x-mpegURL"},
    {"pls", "audio/x-scpls"},
    {"xspf", "application/xspf+xml"},
    {"au", "audio/basic"},
    {"snd", "audio/basic"},
    {"mid", "audio/midi"},
    {"midi", "audio/midi"},
    {"kar", "audio/midi"},
    {"mpga", "audio/mpeg"},
    {"mp2", "audio/mpeg"},
    {"mp2a", "audio/mpeg"},
    {"mp3", "audio/mpeg"},
    {"m2a", "audio/mpeg"},
    {"m3a", "audio/mpeg"},
    {"oga", "audio/ogg"},
    {"ogg", "audio/ogg"},
    {"spx", "audio/ogg"},
    {"opus", "audio/opus"},
    {"ra", "audio/x-realaudio"},
    {"ram", "audio/x-pn-realaudio"},
    {"wav", "audio/x-wav"},
    {"weba", "audio/webm"},
    {"aac", "audio/aac"},
    {"flac", "audio/flac"},
    {"mka", "audio/x-matroska"},
    {"mkv", "video/x-matroska"},
    {"ogv", "video/ogg"},
    {"qt", "video/quicktime"},
    {"mov", "video/quicktime"},
    {"webm", "video/webm"},
    {"rm", "application/vnd.rn-realmedia"},
    {"rpm", "application/x-rpm"},
    {"rar", "application/x-rar-compressed"},
    {"deb", "application/x-debian-package"},
    {"dmg", "application/x-apple-diskimage"},
    {"iso", "application/x-iso9660-image"},
    {"bin", "application/octet-stream"},
    {"dat", "application/octet-stream"},
    {"dump", "application/octet-stream"},
    {"exe", "application/octet-stream"},
    {"img", "application/octet-stream"},
    {"msi", "application/octet-stream"},
    {"msp", "application/octet-stream"},
    {"msu", "application/octet-stream"},
    {"class", "application/java-vm"},
    {"jar", "application/java-archive"},
    {"war", "application/java-archive"},
    {"ear", "application/java-archive"},
    {"jad", "text/vnd.sun.j2me.app-descriptor"},
    {"jardiff", "application/x-java-archive-diff"},
    {"jnlp", "application/x-java-jnlp-file"},
    {"ser", "application/java-serialized-object"},
    {"class", "application/java-vm"},
    {"js", "application/javascript"},
    {"mjs", "application/javascript"},
    {"json", "application/json"},
    {"map", "application/json"},
    {"jsonld", "application/ld+json"},
    {"webmanifest", "application/manifest+json"},
    {"xhtml", "application/xhtml+xml"},
    {"xht", "application/xhtml+xml"},
    {"xslt", "application/xslt+xml"},
    {"xsl", "application/xslt+xml"},
    {"xspf", "application/xspf+xml"},
    {"xul", "application/vnd.mozilla.xul+xml"},
    {"yaml", "text/yaml"},
    {"yml", "text/yaml"},
};


std::string http::MimeType::ConvertToMimeType(const std::string &type)
{
    auto it = mimeTypeMap.find(type);
    if( it == mimeTypeMap.end() ) {
        return "text/plain";
    }
    return it->second;
}

bool
HeaderPair::HasKey(const std::string& key) const
{
    return m_headerPairs.contains(key);
}

std::string 
HeaderPair::GetValue(const std::string& key)
{
    if (m_headerPairs.contains(key))
        return m_headerPairs[key];
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
    this->m_headerPairs.emplace(key, value);
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
    for (auto& [k, v] : m_headerPairs)
    {
        m_stream << k << ": " << v << "\r\n";
    }
    std::string result = m_stream.str();
    m_stream.str("");
    return result;
}

void HeaderPair::Clear()
{
    if(!m_headerPairs.empty()) m_headerPairs.clear();
}

HeaderPair &HeaderPair::operator=(const HeaderPair &headerPair)
{
    m_stream << headerPair.m_stream.str();
    m_headerPairs = headerPair.m_headerPairs;
    return *this;
}

HttpMethod& 
HttpRequestHeader::Method()
{
    return this->m_method;
}

std::string& 
HttpRequestHeader::Uri()
{
    return this->m_uri;
}

HttpVersion& 
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
HttpRequestHeader::FromString(HttpDecodeStatus& status, std::string_view str, size_t& next_index)
{
    std::string_view method, version, key, value;
    size_t n = str.size();
    size_t i = next_index, version_begin = 0, key_begin = 0, value_begin = 0;
    for (; i < n; ++i)
    {
        if(status == HttpDecodeStatus::kHttpHeadEnd) break;
        switch (status)
        {
        case HttpDecodeStatus::kHttpHeadMethod:
        {
            if(str[i] == ' ')
            {
                method = std::string_view(str.data(), i);
                this->m_method = StringToHttpMethod(method);
                status = HttpDecodeStatus::kHttpHeadUri;
                next_index = i + 1;
            }
        }
        break;
        case HttpDecodeStatus::kHttpHeadUri:
        {
            if (str[i] != ' ')
            {
                m_uri += str[i];
            }
            else
            {
                if (m_uri.length() > HTTP_URI_MAX_LEN)
                {
                    LogError("[Uri is too long, Uri:{}]", this->m_uri);
                    return error::kHttpError_UriTooLong;
                }
                this->m_uri = ConvertFromUri(std::move(this->m_uri), false);
                ParseArgs(this->m_uri);
                status = HttpDecodeStatus::kHttpHeadVersion;
                next_index = i + 1;
            }
        }
        break;
        case HttpDecodeStatus::kHttpHeadVersion:
        {
            if(version_begin == 0) version_begin = i;
            if(str[i] == '\r' && i < n - 1 && str[i + 1] == '\n')
            {
                version = std::string_view(str.data() + version_begin, i - version_begin);
                this->m_version = StringToHttpVersion(version);
                ++i;
                status = HttpDecodeStatus::kHttpHeadKey;
                next_index = i + 1;
            }
        }
        break;
        case HttpDecodeStatus::kHttpHeadKey:
        {
            if (key_begin == 0) key_begin = i;
            if (str[i] == '\r' && i < n - 1 && str[i + 1] == '\n')
            {
                next_index = i + 2;
                status = HttpDecodeStatus::kHttpHeadEnd;
            }
            else
            {
                if(str[i] == ':')
                {
                    key = std::string_view(str.data() + key_begin, i - key_begin);
                    key_begin = 0;
                    if (i + 1 < n && str[i + 1] == ' ')
                        ++i;
                    status = HttpDecodeStatus::kHttpHeadValue;
                    next_index = i + 1;
                }
            }
        }
        break;
        case HttpDecodeStatus::kHttpHeadValue:
        {
            if (value_begin == 0) value_begin = i;
            if(str[i] == '\r' && i < n - 1 && str[i + 1] == '\n')
            {
                value = std::string_view(str.data() + value_begin, i - value_begin);
                value_begin = 0;
                m_headerPairs.AddHeaderPair(std::string(key), std::string(value));
                ++i;
                status = HttpDecodeStatus::kHttpHeadKey;
                next_index = i + 1;
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
    m_stream << HttpMethodToString(this->m_method) << " ";
    std::ostringstream url;
    url << m_uri;
    if (!m_argList.empty())
    {
        url << '?';
        int i = 0;
        for (auto& [k, v] : m_argList)
        {
            url << k << '=' << v ;
            if(i++ < m_argList.size() - 1) {
                url << '&';
            }
        }
    }
    m_stream << ConvertToUri(url.str());
    m_stream << " " << HttpVersionToString(this->m_version) << "\r\n" << m_headerPairs.ToString() << "\r\n";
    std::string result = m_stream.str();
    m_stream.str("");
    return result;
}

void 
HttpRequestHeader::CopyFrom(const HttpRequestHeader::ptr& header)
{
    this->m_method = header->m_method;
    this->m_uri = header->m_uri;
    this->m_version = header->m_version;
    this->m_argList = header->m_argList;
    this->m_headerPairs = header->m_headerPairs;
}

void HttpRequestHeader::Reset()
{
    m_version = HttpVersion::Http_Version_Unknown;
    m_method = HttpMethod::Http_Method_Unknown;
    if(!m_uri.empty()) m_uri.clear();
    if(!m_argList.empty()) m_argList.clear();
    m_headerPairs.Clear();
}

void 
HttpRequestHeader::ParseArgs(const std::string& uri)
{
    size_t argindx = uri.find('?');
    if (argindx != std::string::npos)
    {
        int cur = 0;
        this->m_uri = uri.substr(cur, argindx - cur);
        const std::string args = uri.substr(argindx + 1);
        std::string key, value;
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
HttpRequestHeader::IsHex(const char c, int& v)
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
HttpRequestHeader::ToUtf8(const int code, char* buff)
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


std::pair<bool,size_t> 
HttpRequest::DecodePdu(const std::string_view& buffer)
{
    m_error->Reset();
    size_t n = buffer.length();
    //header
    if(m_status < HttpDecodeStatus::kHttpHeadEnd) 
    {
        error::HttpErrorCode errCode = m_header->FromString(m_status, buffer, m_next_index);
        if( errCode != error::kHttpError_NoError ) {
            m_error->Code() = errCode;
            return {false, 0};
        }
        if( m_next_index > HTTP_HEADER_MAX_LEN ) {
            LogError("[Header is too long, Header Len: {} Bytes]", buffer.length());
            m_error->Code() = error::kHttpError_HeaderTooLong;
            return {false, 0};
        } 
        if( m_status != HttpDecodeStatus::kHttpHeadEnd) {
            m_error->Code() = error::kHttpError_HeaderInComplete;
            return {false, 0};
        }
    } 

    if(m_status >= HttpDecodeStatus::kHttpHeadEnd) 
    {
        if((m_header->HeaderPairs().HasKey("Transfer-Encoding") && 0 == m_header->HeaderPairs().GetValue("Transfer-Encoding").compare("chunked"))
            || (m_header->HeaderPairs().HasKey("transfer-encoding") && 0 == m_header->HeaderPairs().GetValue("transfer-encoding").compare("chunked")))
        {
            if(GetChunkBody(buffer)){
                m_status = HttpDecodeStatus::kHttpBody;
            }  else {
                return {false, 0};
            }
        }
        else if (m_header->HeaderPairs().HasKey("Content-Length") || m_header->HeaderPairs().HasKey("content-length"))
        {
            if(GetHttpBody(buffer)) {
                m_status = HttpDecodeStatus::kHttpBody;
            } else {
                return {false, 0};
            }
        }
    }
    size_t length = m_next_index;
    m_next_index = 0;
    return {true, length};
}

std::string 
HttpRequest::EncodePdu() const
{
    if((m_header->HeaderPairs().HasKey("Transfer-Encoding") &&  "chunked" == m_header->HeaderPairs().GetValue("Transfer-Encoding") )||
        (m_header->HeaderPairs().HasKey("transfer-encoding") && "chunked" == m_header->HeaderPairs().GetValue("transfer-encoding"))){
        return m_header->ToString();
    }
    if(!m_body.empty() && !m_header->HeaderPairs().HasKey("Content-Length") && !m_header->HeaderPairs().HasKey("content-length")){
        m_header->HeaderPairs().AddHeaderPair("Content-Length", std::to_string(m_body.length()));
    }
    return std::move(m_header->ToString() + m_body);
}

int HttpRequest::GetErrorCode() const
{
    return m_error->Code();
}

std::string HttpRequest::GetErrorString()
{
    return m_error->ToString(m_error->Code());
}

void HttpRequest::Reset()
{
    m_header->Reset();
    m_error->Reset();
    if(!m_body.empty()) m_body.clear();
    m_status = HttpDecodeStatus::kHttpHeadMethod;
    m_next_index = 0;
}

bool HttpRequest::HasError() const
{
    return m_error->HasError();
}

bool 
HttpRequest::StartChunk()
{
    if((m_header->HeaderPairs().HasKey("transfer-encoding") || m_header->HeaderPairs().HasKey("Transfer-Encoding")) && 
        ("chunked" == m_header->HeaderPairs().GetValue("transfer-encoding") || "chunked" == m_header->HeaderPairs().GetValue("Transfer-Encoding"))){
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
HttpRequest::ToChunkData(std::string&& buffer)
{
    size_t length = buffer.length();
    std::string res = std::to_string(length);
    res += "\r\n";
    res += buffer;
    res += "\r\n";
    return std::move(res);
}

std::string 
HttpRequest::EndChunk()
{
    return "0\r\n\r\n";
}

HttpRequest::HttpRequest()
    :m_next_index(0), m_status(HttpDecodeStatus::kHttpHeadMethod)
{
    this->m_error = std::make_shared<error::HttpError>();
    this->m_header = std::make_shared<HttpRequestHeader>();
}


HttpRequestHeader::ptr 
HttpRequest::Header()
{
    if(!m_header) m_header = std::make_shared<HttpRequestHeader>();
    return m_header;
}

void http::HttpRequest::SetContent(const std::string& type, std::string &&content)
{
    m_header->HeaderPairs().AddHeaderPair("Content-Type", MimeType::ConvertToMimeType(type));
    m_body = std::move(content);
}

std::string_view
HttpRequest::GetContent()
{
    return this->m_body;
}

bool 
HttpRequest::GetHttpBody(const std::string_view &buffer)
{
    size_t n = buffer.length();
    if(m_header->HeaderPairs().HasKey("content-length") || m_header->HeaderPairs().HasKey("Content-Length")){
        std::string contentLength = m_header->HeaderPairs().GetValue("Content-Length");
        if( contentLength.empty() ) contentLength = m_header->HeaderPairs().GetValue("Content-Length");
        size_t length = 0;
        try
        {
            length = std::stoul(contentLength);
        }
        catch(const std::exception& e)
        {
            LogError("[Content-length is not Integer, content-length:{}]]", contentLength);
            m_error->Code() = error::kHttpError_BadRequest;

        }
        
        if(length + m_next_index <= n) {
            m_body = buffer.substr(m_next_index, length);
            m_next_index += length;
            if(m_next_index + 4 < n && buffer.substr(m_next_index,4) == "\r\n\r\n") {
                m_next_index += 4;
            }  
            LogTrace("[Body is completed, Len:{} Bytes, Body Package:{}]", length , this->m_body);
        }else{
            LogWarn("[body is incomplete, len:{} Bytes, expect {} Bytes]", n, length);
            m_error->Code() = error::kHttpError_BodyInComplete;
            return false;
        }
    }else{
        size_t pos = buffer.find("\r\n\r\n", m_next_index);
        if(pos == std::string::npos){
            if(!buffer.empty()){
                LogWarn("[Body is incomplete, header not has length and not end with '\\r\\n\\r\\n']");
                m_error->Code() = error::kHttpError_BodyInComplete;
                return false;
            }
        }else{
            m_body = buffer.substr(m_next_index, pos - m_next_index);
            m_next_index = pos + 4;
        }
    }
    return true;
}

bool 
HttpRequest::GetChunkBody(const std::string_view& buffer)
{
    while (!buffer.empty())
    {
        int pos = buffer.find("\r\n", m_next_index);
        std::string_view temp = buffer.substr(m_next_index, pos - m_next_index);
        int length;
        try
        {
            length = std::stoi(std::string(temp.cbegin(), temp.length()));
        }
        catch (const std::exception &e)
        {
            LogError("[Chunck is Illegal, ErrMsg:{}]", e.what());
            m_error->Code() = error::kHttpError_ChunckHasError;
            return false;
        }
        if(length == 0){
            m_next_index = pos + 4;
            LogTrace("[Chunck is finished, Chunck Len:{} Bytes]", pos+4);
            break;
        }else if(length + 4 + pos > buffer.length()){
            LogDebug("[Chunck is incomplete, Chunck Len:{} Bytes, Buffer Len:{} Bytes]", length + pos + 4, buffer.length());
            m_error->Code() = error::kHttpError_BodyInComplete;
            return false;
        }
        this->m_body += buffer.substr(pos+2,length);
        m_next_index = pos + 4 + length;
    }
    return true;
}


HttpVersion& 
HttpResponseHeader::Version()
{
    return this->m_version;
}

HttpStatusCode& 
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
    m_stream <<  HttpVersionToString(m_version) << ' ' << std::to_string(static_cast<int>(this->m_code)) << ' ' << HttpStatusCodeToString(m_code) << "\r\n" \
        << m_headerPairs.ToString() << "\r\n";
    std::string result = m_stream.str();
    m_stream.str("");
    return result;
}

error::HttpErrorCode 
HttpResponseHeader::FromString(HttpDecodeStatus& status, std::string_view str, size_t& next_index)
{
    size_t n = str.length();
    std::string_view key;
    size_t i = next_index, code_begin = 0, key_begin = 0, value_begin = 0;
    for (; i < n; ++i)
    {
        if (status == HttpDecodeStatus::kHttpHeadEnd)
            break;
        switch (status)
        {
        case HttpDecodeStatus::kHttpHeadVersion:
        {
            if( str[i] == ' ' )
            {
                m_version = StringToHttpVersion(std::string_view(str.data(), i));
                status = HttpDecodeStatus::kHttpHeadStatusCode;
                next_index = i + 1;
            }
        }
        break;
        case HttpDecodeStatus::kHttpHeadStatusCode:
        {
            if ( code_begin == 0 ) code_begin = i;
            if( str[i] == ' ' )
            {
                int code;
                try
                {
                    code = std::stoi(std::string(str.data() + code_begin, i - code_begin));
                }
                catch (std::invalid_argument &e)
                {
                    LogError("[Http status code is illegal]");
                    return error::kHttpError_HttpCodeInvalid;
                }
                m_code = static_cast<HttpStatusCode>(code);
                status = HttpDecodeStatus::kHttpHeadStatusMsg;
                next_index = i + 1;
            }
        }
        break;
        case HttpDecodeStatus::kHttpHeadStatusMsg:
        {
            if (str[i] == '\r' && i < n - 1 && str[i + 1] == '\n')
            {
                status = HttpDecodeStatus::kHttpHeadKey;
                ++i;
                next_index = i + 1;
            }
        }
        break;
        case HttpDecodeStatus::kHttpHeadKey:
        {
            if (key_begin == 0) key_begin = i;
            if (str[i] == '\r' && i < n - 1 && str[i + 1] == '\n')
            {
                next_index = i + 2;
                status = HttpDecodeStatus::kHttpHeadEnd;
            }
            else
            {
                if(str[i] == ':')
                {
                    key = std::string_view(str.data() + key_begin, i - key_begin);
                    key_begin = 0;
                    if (i + 1 < n && str[i + 1] == ' ')
                        ++i;
                    status = HttpDecodeStatus::kHttpHeadValue;
                    next_index = i + 1;
                }
            }
        }
        break;
        case HttpDecodeStatus::kHttpHeadValue:
        {
            if (value_begin == 0) value_begin = i;
            if(str[i] == '\r' && i < n - 1 && str[i + 1] == '\n')
            {
                std::string_view value = std::string_view(str.data() + value_begin, i - value_begin);
                value_begin = 0;
                m_headerPairs.AddHeaderPair(std::string(key), std::string(value));
                ++i;
                status = HttpDecodeStatus::kHttpHeadKey;
                next_index = i + 1;
            }
        }
        break;
        default:
            break;
        }
    }
    return error::kHttpError_NoError;
}

HttpResponse::HttpResponse()
    :m_next_index(0), m_status(HttpDecodeStatus::kHttpHeadVersion)
{
    this->m_error = std::make_shared<error::HttpError>();
    this->m_header = std::make_shared<HttpResponseHeader>();
}


HttpResponseHeader::ptr 
HttpResponse::Header()
{
    if(!m_header) m_header = std::make_shared<HttpResponseHeader>();
    return m_header;
}

void http::HttpResponse::SetContent(const std::string& type, std::string &&content)
{
    m_header->HeaderPairs().AddHeaderPair("Content-Type", MimeType::ConvertToMimeType(type));
    m_body = std::move(content);
}

std::string_view 
HttpResponse::GetContent()
{
    return m_body;
}

std::string
HttpResponse::EncodePdu() const
{
    if((m_header->HeaderPairs().HasKey("Transfer-Encoding") || m_header->HeaderPairs().HasKey("transfer-encoding")) && 
        ( 0 == m_header->HeaderPairs().GetValue("Transfer-Encoding").compare("chunked") || 0 == m_header->HeaderPairs().GetValue("transfer-encoding").compare("chunked") )){
        return m_header->ToString();
    }
    if(!m_body.empty() && !m_header->HeaderPairs().HasKey("Content-Length") && !m_header->HeaderPairs().HasKey("content-length")){
        m_header->HeaderPairs().AddHeaderPair("Content-Length", std::to_string(m_body.length()));
    }
    std::string header = m_header->ToString();
    return std::move( header + m_body);
}


std::pair<bool,size_t> 
HttpResponse::DecodePdu(const std::string_view& buffer)
{
    m_error->Reset();
    //header
    if(m_status < HttpDecodeStatus::kHttpHeadEnd) 
    {
        const error::HttpErrorCode errCode = m_header->FromString(m_status, buffer, m_next_index);
        if( errCode != error::kHttpError_NoError ) {
            m_error->Code() = errCode;
            return {false, 0};
        }
        if( m_next_index > HTTP_HEADER_MAX_LEN ) {
            LogError("[Header is too long, Header Len: {} Bytes]", buffer.length());
            m_error->Code() = error::kHttpError_HeaderTooLong;
            return {false, 0};
        } 
        if( m_status != HttpDecodeStatus::kHttpHeadEnd) {
            m_error->Code() = error::kHttpError_HeaderInComplete;
            return {false, 0};
        }
    } 

    if(m_status >= HttpDecodeStatus::kHttpHeadEnd) 
    {
        if((m_header->HeaderPairs().HasKey("Transfer-Encoding") && 0 == m_header->HeaderPairs().GetValue("Transfer-Encoding").compare("chunked"))
            || (m_header->HeaderPairs().HasKey("transfer-encoding") && 0 == m_header->HeaderPairs().GetValue("transfer-encoding").compare("chunked")))
        {
            if(GetChunckBody(buffer)){
                m_status = HttpDecodeStatus::kHttpBody;
            }  else {
                return {false, 0};
            }
        }
        else if (m_header->HeaderPairs().HasKey("Content-Length") || m_header->HeaderPairs().HasKey("content-length"))
        {
            if(GetHttpBody(buffer)) {
                m_status = HttpDecodeStatus::kHttpBody;
            } else {
                return {false, 0};
            }
        }
    }
    size_t length = m_next_index;
    m_next_index = 0;
    return {true, length};
}

bool HttpResponse::HasError() const
{
    return m_error->HasError();
}

int HttpResponse::GetErrorCode() const
{
    return m_error->Code();
}

std::string HttpResponse::GetErrorString()
{
    return m_error->ToString(m_error->Code());
}

void HttpResponse::Reset()
{
    m_error->Reset();
    m_header.reset();
    m_body.clear();
    m_status = HttpDecodeStatus::kHttpHeadVersion;
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


bool 
HttpResponse::GetHttpBody(const std::string_view& buffer)
{
    size_t n = buffer.length();
    if(m_header->HeaderPairs().HasKey("content-length") || m_header->HeaderPairs().HasKey("Content-Length")){
        std::string contentLength = m_header->HeaderPairs().GetValue("Content-Length");
        if( contentLength.empty() ) contentLength = m_header->HeaderPairs().GetValue("Content-Length");
        size_t length;
        try
        {
            length = std::stoul(contentLength);
        }
        catch(const std::exception& e)
        {
            LogError("[Content-length is not a number, content-length:{}]", contentLength);
            m_error->Code() = error::kHttpError_BadRequest;

        }
        
        if(length + m_next_index <= n) {
            m_body = buffer.substr(m_next_index, length);
            m_next_index += length;
            if(m_next_index + 4 < n && buffer.substr(m_next_index,4).compare("\r\n\r\n") == 0) {
                m_next_index += 4;
            }  
            LogTrace("[Body is completed, Body Len:{} Bytes, Body Package:{}]", length , this->m_body);
        }else{
            LogWarn("[Body is incomplete, Body len:{} Bytes, expect {} Bytes]", n, length);
            m_error->Code() = error::kHttpError_BodyInComplete;
            return false;
        }
    }else{
        size_t pos = buffer.find("\r\n\r\n", m_next_index);
        if(pos == std::string::npos){
            if(!buffer.empty()){
                LogWarn("[Body is incomplete, header not has length and not end with '\\r\\n\\r\\n']]");
                m_error->Code() = error::kHttpError_BodyInComplete;
                return false;
            }
        }else{
            m_body = buffer.substr(m_next_index, pos - m_next_index);
            m_next_index = pos + 4;
        }
    }
    return true;
}


bool 
HttpResponse::GetChunckBody(const std::string_view& buffer)
{
    while (!buffer.empty())
    {
        int pos = buffer.find("\r\n", m_next_index);
        std::string_view temp = buffer.substr(m_next_index, pos - m_next_index);
        int length;
        try
        {
            length = std::stoi(std::string(temp.cbegin(), temp.length()));
        }
        catch (const std::exception &e)
        {
            LogError("[Chunck is Illegal, ErrMsg:{}]", e.what());
            m_error->Code() = error::kHttpError_ChunckHasError;
            return false;
        }
        if(length == 0){
            m_next_index = pos + 4;
            LogTrace("[Chunck is finished, Chunck Len:{} Bytes]", pos+4);
            break;
        }else if(length + 4 + pos > buffer.length()){
            LogTrace("[Chunck is incomplete, Chunck Len:{} Bytes, Buffer Len:{} Bytes]", length + pos + 4, buffer.length());
            m_error->Code() = error::kHttpError_BodyInComplete;
            return false;
        }
        this->m_body += buffer.substr(pos+2,length);
        m_next_index = pos + 4 + length;
    }
    return true;
}

//function
bool 
FormDataHeader::HasKey(const std::string& key)
{
    return m_headerPairs.contains(key);
}

std::string 
FormDataHeader::GetValue(const std::string& key)
{
    if(m_headerPairs.contains(key)) return m_headerPairs[key];
    return "";
}

bool 
FormDataHeader::RemoveHeaderPair(const std::string& key)
{
    if(m_headerPairs.contains(key)) {
        m_headerPairs.erase(key);
        return true;
    }
    return false;
}

bool  
FormDataHeader::SetHeaderPair(const std::string& key, const std::string& value)
{
    if(m_headerPairs.contains(key)){
        m_headerPairs[key] = value;
        return true;
    }
    return false;
}

bool 
FormDataHeader::AddHeaderPair(const std::string& key, const std::string& value)
{
    if(!m_headerPairs.contains(key)){
        m_headerPairs[key] = value;
        return true;
    }
    return false;
}

std::string 
FormDataHeader::ToString()
{
    std::string res;
    for(auto &headerPair : m_headerPairs)
    {
        res += headerPair.first + ": " + headerPair.second + "\r\n";
    }
    return res + "\r\n";
}

void 
FormDataValue::SetValue(const FormFile& file)
{
    this->m_body = file;
    this->m_type = kFormDataType_File;
}

bool 
FormDataValue::IsFile() const
{
    return this->m_type == kFormDataType_File;
}

FormFile 
FormDataValue::ToFile() const
{
    return std::get<FormFile>(this->m_body);
}

void 
FormDataValue::SetValue(const std::string& body)
{
    this->m_body = body;
    this->m_type = kFormDataType_String;
}

bool 
FormDataValue::IsString() const
{
    return this->m_type == kFormDataType_String;
}

std::string 
FormDataValue::ToString() const
{
    return std::get<std::string>(this->m_body);
}

void 
FormDataValue::SetValue(int number)
{
    this->m_body = number;
    this->m_type = kFormDataType_Number;
}

bool 
FormDataValue::IsNumber() const
{
    return this->m_type == kFormDataType_Number;
}

void 
FormDataValue::SetValue(double number)
{
    this->m_body = number;
    this->m_type = kFormDataType_Double;
}

int 
FormDataValue::ToNumber() const
{
    return std::get<int>(this->m_body);
}

bool 
FormDataValue::IsDouble() const
{
    return this->m_type == kFormDataType_Double;
}

double 
FormDataValue::ToDouble() const
{
    return std::get<double>(m_body);
}

FormDataHeader& 
FormDataValue::Header()
{
    return this->m_header;
}

std::string 
FormDataValue::Name() const
{
    return this->m_name;
}

void 
FormDataValue::SetName(const std::string& name)
{
    this->m_name = name;
}

bool
HttpFormDataHelper::IsFormData(HttpRequest::ptr request)
{
    std::string contentType = request->Header()->HeaderPairs().GetValue("Content-Type");
    int pos = contentType.find("multipart/form-data");
    if(pos != std::string::npos) return true;
    return false;
}

bool 
HttpFormDataHelper::ParseFormData(HttpRequest::ptr request, std::vector<FormDataValue>& values)
{
    std::string contentType = request->Header()->HeaderPairs().GetValue("Content-Type");
    int begin = contentType.find("boundary=");
    if(begin != std::string::npos)
    {
        std::string boundary = contentType.substr(begin + 9);
        std::string_view body = request->GetContent();
        std::vector<std::string_view> parts = utils::StringSplitter::SpiltWithStr(body, "--" + boundary + "\r\n");
        size_t len = parts[parts.size() - 1].find("--" + boundary + "--");
        parts[parts.size() - 1] = parts[parts.size() - 1].substr(0, len);
        for(auto &part : parts)
        {
            FormDataValue temp;
            size_t eLength = 0;
            std::string_view rowstr;
            //解析头
            do
            {
                //取出一行
                size_t now = part.find("\r\n", eLength);
                rowstr = part.substr(eLength, now - eLength);
                eLength = (now + 2);
                if(rowstr.empty()) break;
                //分离key和value
                size_t des = rowstr.find_first_of(": ");
                std::string_view key = rowstr.substr(0, des);
                std::string_view value = rowstr.substr(des + 2);
                temp.Header().AddHeaderPair(std::string(key.data(), key.length()), std::string(value.data(), value.length()));
            } while (eLength < part.length());
            //解析体
            std::string disposition = temp.Header().GetValue("Content-Disposition");
            if(!disposition.empty())
            {
                size_t namePosBeg = disposition.find("name=");
                size_t namePosEnd = disposition.find('"', namePosBeg + 6);
                temp.SetName(disposition.substr(namePosBeg + 6, namePosEnd - namePosBeg - 6 ));
                size_t pos = disposition.find("filename=");
                //file
                if(pos != std::string::npos)
                {
                    std::string filename = disposition.substr(pos + 10, disposition.length() - pos - 11);
                    std::string_view body = part.substr(eLength, part.length() - eLength - 2);
                    std::string tfile = std::string(filename.data(), filename.length());
                    FormFile file;
                    file.m_fileName = std::move(tfile);
                    file.m_body = std::move(body);
                    temp.SetValue(file);
                }
                else
                {
                    std::string_view x = part.substr(eLength, part.length() - eLength - 2);
                    std::string body = std::string(x.data(), x.length());
                    std::regex numpattern("^[-+]?\\d+$");
                    if(std::regex_match(body, numpattern))
                    {
                        temp.SetValue(std::stoi(body));
                    }
                    else
                    {
                        std::regex doublepattern("^[-+]?\\d+\\.\\d+$");
                        if(std::regex_match(body, doublepattern))
                        {
                            temp.SetValue(std::stod(body));
                        }
                        else
                        {
                            temp.SetValue(body);
                        }
                    }
                }
                values.push_back(temp);
            }
                
        }

    }
    else
    {
        return false;
    }
    return true;
}


void 
HttpFormDataHelper::FormDataToString(HttpRequest::ptr request, const std::string& boundary, const std::vector<FormDataValue>& values)
{
    std::ostringstream stream;
    for(int i = 0 ; i < values.size() ; ++ i)
    {
        stream << "--" << boundary << "\r\n";
        if( values[i].IsFile() )
        {
            auto file = values[i].ToFile();
            stream << "Content-Disposition: form-data; name=\"" << values[i].Name() << "\"; filename=\"" << file.m_fileName << "\"\r\n";
            stream << "Content-Type: application/octet-stream\r\n\r\n";
            stream << file.m_body << "\r\n";
        }
        else if( values[i].IsNumber() )
        {
            stream << "Content-Disposition: form-data; name=\"" << values[i].Name() << "\"\r\n\r\n";
            stream << std::to_string(values[i].ToNumber()) << "\r\n";
        }
        else if( values[i].IsDouble() )
        {
            stream << "Content-Disposition: form-data; name=\"" << values[i].Name() << "\"\r\n\r\n";
            std::string num = std::to_string(values[i].ToDouble());
            stream << num << "\r\n";
        }
        else if( values[i].IsString() )
        {
            stream << "Content-Disposition: form-data; name=\"" << values[i].Name() << "\"\r\n\r\n";
            stream << values[i].ToString() << "\r\n";
        }
    }
    stream << "--" << boundary << "--\r\n\r\n";
    request->SetContent("", stream.str());
    if(request->Header()->HeaderPairs().HasKey("Content-Type"))
    {
        request->Header()->HeaderPairs().SetHeaderPair("Content-Type", "multipart/form-data; boundary=" + boundary);
    }
    else
    {
        request->Header()->HeaderPairs().AddHeaderPair("Content-Type", "multipart/form-data; boundary=" + boundary);
    }
}

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
    response->Header()->HeaderPairs().AddHeaderPair("Server", "galay");
    response->Header()->HeaderPairs().AddHeaderPair("Date", utils::GetCurrentGMTTimeString());
    if(!body.empty()) response->SetContent(type, std::move(body));
    return true;
}


}