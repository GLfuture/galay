#include "HttpProtoc.hpp"
#include "assert.h"

namespace galay::http 
{

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

bool HttpRequest::ParseHeader(const std::string_view& buffer)
{
    m_error->Reset();
    error::HttpErrorCode errCode = m_header->FromString(m_status, buffer, m_next_index);
    if( errCode != error::kHttpError_NoError ) {
        m_error->Code() = errCode;
        return false;
    }
    if( m_next_index > HTTP_HEADER_MAX_LEN ) {
        m_error->Code() = error::kHttpError_HeaderTooLong;
        return false;
    } 
    if( m_status != HttpDecodeStatus::kHttpHeadEnd) {
        m_error->Code() = error::kHttpError_HeaderInComplete;
        return false;
    }
    return true;
}

bool HttpRequest::ParseBody(const std::string_view &buffer)
{
    if((m_header->HeaderPairs().HasKey("Transfer-Encoding") && 0 == m_header->HeaderPairs().GetValue("Transfer-Encoding").compare("chunked"))
        || (m_header->HeaderPairs().HasKey("transfer-encoding") && 0 == m_header->HeaderPairs().GetValue("transfer-encoding").compare("chunked")))
    {
        if(GetChunkBody(buffer)){
            m_status = HttpDecodeStatus::kHttpBody;
        }  else {
            return false;
        }
    }
    else if (m_header->HeaderPairs().HasKey("Content-Length") || m_header->HeaderPairs().HasKey("content-length"))
    {
        if(GetHttpBody(buffer)) {
            m_status = HttpDecodeStatus::kHttpBody;
        } else {
            return false;
        }
    }
    size_t length = m_next_index;
    m_next_index = 0;
    return true;
}

std::string 
HttpRequest::EncodePdu() const
{
    if((m_header->HeaderPairs().HasKey("Transfer-Encoding") &&  "chunked" == m_header->HeaderPairs().GetValue("Transfer-Encoding") )||
        (m_header->HeaderPairs().HasKey("transfer-encoding") && "chunked" == m_header->HeaderPairs().GetValue("transfer-encoding"))){
        return m_header->ToString();
    }
    if(!m_body.empty()){
        if(!m_body.empty() ){
            if(!m_header->HeaderPairs().HasKey("Content-Length") && !m_header->HeaderPairs().HasKey("content-length")) {
                m_header->HeaderPairs().AddHeaderPair("Content-Length", std::to_string(m_body.length()));
            } else {
                if(m_header->HeaderPairs().HasKey("Content-Length")) {
                    m_header->HeaderPairs().SetHeaderPair("Content-Length", std::to_string(m_body.length()));
                } else {
                    m_header->HeaderPairs().SetHeaderPair("content-length", std::to_string(m_body.length()));
                }
            }
        }
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

void http::HttpRequest::SetContent(std::string &&content, const std::string& type)
{
    if(!type.empty()) m_header->HeaderPairs().AddHeaderPair("Content-Type", MimeType::ConvertToMimeType(type));
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
            m_error->Code() = error::kHttpError_BadRequest;
        }
        
        if(length + m_next_index <= n) {
            m_body = buffer.substr(m_next_index, length);
            m_next_index += length;
            if(m_next_index + 4 < n && buffer.substr(m_next_index,4) == "\r\n\r\n") {
                m_next_index += 4;
            }  
        }else{
            m_error->Code() = error::kHttpError_BodyInComplete;
            return false;
        }
    }else{
        size_t pos = buffer.find("\r\n\r\n", m_next_index);
        if(pos == std::string::npos){
            if(!buffer.empty()){
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
            m_error->Code() = error::kHttpError_ChunckHasError;
            return false;
        }
        if(length == 0){
            m_next_index = pos + 4;
            break;
        }else if(length + 4 + pos > buffer.length()){
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
    if(!m_body.empty() ){
        if(!m_header->HeaderPairs().HasKey("Content-Length") && !m_header->HeaderPairs().HasKey("content-length")) {
            m_header->HeaderPairs().AddHeaderPair("Content-Length", std::to_string(m_body.length()));
        } else {
            if(m_header->HeaderPairs().HasKey("Content-Length")) {
                m_header->HeaderPairs().SetHeaderPair("Content-Length", std::to_string(m_body.length()));
            } else {
                m_header->HeaderPairs().SetHeaderPair("content-length", std::to_string(m_body.length()));
            }
        }
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
            m_error->Code() = error::kHttpError_BadRequest;

        }
        
        if(length + m_next_index <= n) {
            m_body = buffer.substr(m_next_index, length);
            m_next_index += length;
            if(m_next_index + 4 < n && buffer.substr(m_next_index,4).compare("\r\n\r\n") == 0) {
                m_next_index += 4;
            }  
        }else{
            m_error->Code() = error::kHttpError_BodyInComplete;
            return false;
        }
    }else{
        size_t pos = buffer.find("\r\n\r\n", m_next_index);
        if(pos == std::string::npos){
            if(!buffer.empty()){
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
            m_error->Code() = error::kHttpError_ChunckHasError;
            return false;
        }
        if(length == 0){
            m_next_index = pos + 4;
            break;
        }else if(length + 4 + pos > buffer.length()){
            m_error->Code() = error::kHttpError_BodyInComplete;
            return false;
        }
        this->m_body += buffer.substr(pos+2,length);
        m_next_index = pos + 4 + length;
    }
    return true;
}

}