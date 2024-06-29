#include "http1_1.h"
#include <spdlog/spdlog.h>


std::string 
galay::protocol::http::Http1_1_Protocol::GetVersion()
{
    return this->m_version;
}

void 
galay::protocol::http::Http1_1_Protocol::SetVersion(const std::string& version)
{
    this->m_version = version;
}

std::string
galay::protocol::http::Http1_1_Protocol::GetBody()
{
    return this->m_body;
}

void 
galay::protocol::http::Http1_1_Protocol::SetBody(std::string&& body)
{
    this->m_body = body;
}

std::string 
galay::protocol::http::Http1_1_Protocol::GetHeadValue(const std::string& key)
{
    auto it = this->m_headers.find(key);
    if (it == this->m_headers.end())
        return "";
    return it->second;
}

void 
galay::protocol::http::Http1_1_Protocol::SetHeadPair(std::pair<std::string, std::string>&& p_head)
{
    this->m_headers[p_head.first] = p_head.second;
}

void 
galay::protocol::http::Http1_1_Protocol::SetHeaders(const std::unordered_map<std::string,std::string>& headers)
{
    for(auto it = headers.begin(); it != headers.end(); ++it){
        this->m_headers[it->first] = it->second;
    }
}

const std::unordered_map<std::string,std::string>& 
galay::protocol::http::Http1_1_Protocol::GetHeaders()
{
    return this->m_headers;
}

std::unordered_set<std::string> 
galay::protocol::http::Http1_1_Request::m_std_methods = {
    "GET", "POST", "PUT", "DELETE", "HEAD", "OPTIONS", "TRACE", "CONNECT" , "PATCH"
};

std::string 
galay::protocol::http::Http1_1_Request::GetArgValue(const std::string& key)
{
    auto it = this->m_arg_list.find(key);
    if (it == this->m_arg_list.end())
        return "";
    return it->second;
}

void 
galay::protocol::http::Http1_1_Request::SetArgPair(std::pair<std::string, std::string>&& p_arg)
{
    this->m_arg_list[p_arg.first] = p_arg.second;
}

std::string 
galay::protocol::http::Http1_1_Request::GetMethod()
{
    return this->m_method;
}

void 
galay::protocol::http::Http1_1_Request::SetMethod(std::string&& method)
{
    this->m_method = method;
}

std::string 
galay::protocol::http::Http1_1_Request::GetUri()
{
    return this->m_uri;
}

void
galay::protocol::http::Http1_1_Request::SetUri(const std::string& uri)
{
    this->m_uri = uri;
}

galay::common::ProtoJudgeType 
galay::protocol::http::Http1_1_Request::DecodePdu(std::string& buffer)
{
    size_t n = buffer.length();
    if(this->m_header_len == 0){
        galay::common::ProtoJudgeType type = ParseHead(buffer);
        if(type != galay::common::ProtoJudgeType::kProtoFinished) return type;
        spdlog::info("[{}:{}] [Head Package:{}]",__FILE__,__LINE__, buffer.substr(0,m_header_len));
        buffer.erase(0,m_header_len);
        n = buffer.length();
    }
    if(m_headers.contains("content-length")){
        size_t length = std::stoul(m_headers["content-length"]);
        if(length <= n) {
            m_body = buffer.substr(0,length);
            int invaild_len = 0;
            for (int k = length; k < n - 1; ++k)
            {
                if (isalpha(buffer[k + 1]))
                    break;
                else
                    ++invaild_len;
            }
            size_t res =  length + invaild_len;
            spdlog::debug("[{}:{}] [[body is completed] [Body Len:{} Bytes] [Body Package:{}]", __FILE__, __LINE__ , length , this->m_body);
            buffer.erase(0,res);
            return galay::common::ProtoJudgeType::kProtoFinished;
        }else{
            spdlog::warn("[{}:{}] [[body is incomplete] [buffer len:{} Bytes]]",__FILE__,__LINE__,n);
            return galay::common::ProtoJudgeType::kProtoIncomplete;
        }
    }else if(m_headers.contains("transfer-encoding") && m_headers["transfer-encoding"] == "chunked"){
        while (!buffer.empty())
        {
            int pos = buffer.find_first_of("\r\n");
            std::string temp = buffer.substr(0,pos);
            int length;
            try
            {
                length = std::stoi(temp);
            }
            catch (const std::exception &e)
            {
                buffer.clear();
                spdlog::error("[{}:{}] [Chunck is Illegal] [ErrMsg:{}]", __FILE__, __LINE__, e.what());
                return galay::common::ProtoJudgeType::kProtoIllegal;
            }
            if(length == 0){
                buffer.erase(0,pos + 4);
                spdlog::debug("[{}:{}] [[Chunck is finished] [Chunck Len:{} Bytes]]",__FILE__,__LINE__,pos+4);
                return galay::common::ProtoJudgeType::kProtoFinished;
            }else if(length + 4 + pos > buffer.length()){
                spdlog::debug("[{}:{}] [[Chunck is incomplete] [Chunck Len:{} Bytes] [Buffer Len:{} Bytes]]",__FILE__,__LINE__,length + pos + 4,buffer.length());
                return galay::common::ProtoJudgeType::kProtoIncomplete;
            }
            this->m_body += buffer.substr(pos+2,length);
            buffer.erase(0,pos + 4 + length);
        }
    } 
    return galay::common::ProtoJudgeType::kProtoFinished;
}

galay::common::ProtoJudgeType 
galay::protocol::http::Http1_1_Request::ParseHead(const std::string &buffer)
{
    size_t n = buffer.length();
    size_t pos = buffer.find("\r\n\r\n");
    if (pos == std::string::npos)
    {
        if (buffer.length() > HTTP_HEADER_MAX_LEN)
        {
            spdlog::error("[{}:{}] [[header is too long] [Header Len: More than {} Bytes]]", __FILE__, __LINE__, buffer.length());
            return galay::common::ProtoJudgeType::kProtoIllegal;
        }
        Clear();
        spdlog::debug("[{}:{}] [[header is incomplete] [Rbuffer Len:{} Bytes]]", __FILE__, __LINE__, buffer.length());
        return galay::common::ProtoJudgeType::kProtoIncomplete;
    }
    else if (pos > HTTP_HEADER_MAX_LEN)
    {
        spdlog::error("[{}:{}] [[header is too long] [Header Len:{} Bytes]]", __FILE__, __LINE__, pos);
        return galay::common::ProtoJudgeType::kProtoIllegal;
    }
    HttpHeadStatus status = HttpHeadStatus::kHttpMethod;
    std::string key, value;
    size_t i;
    for (i = 0; i < n; ++i)
    {
        if (status == HttpHeadStatus::kHttpBody)
            break;
        switch (status)
        {
        case kHttpMethod:
        {
            if (buffer[i] != ' ')
            {
                m_method += buffer[i];
            }
            else
            {
                if (!m_std_methods.contains(m_method))
                {
                    spdlog::error("[{}:{}] [[method is not standard] [Method:{}]]", __FILE__, __LINE__, this->m_method);
                    return galay::common::ProtoJudgeType::kProtoIllegal;
                }
                status = HttpHeadStatus::kHttpUri;
            }
        }
        break;
        case kHttpUri:
        {
            if (buffer[i] != ' ')
            {
                m_uri += buffer[i];
            }
            else
            {
                if (m_uri.length() > HTTP_URI_MAX_LEN)
                {
                    spdlog::error("[{}:{}] [[uri is too long] [Uri:{}]]", __FILE__, __LINE__, this->m_uri);
                    return galay::common::ProtoJudgeType::kProtoIllegal;
                }
                ConvertUri(m_uri);
                status = HttpHeadStatus::kHttpVersion;
            }
        }
        break;
        case kHttpVersion:
        {
            if (buffer[i] != '\r')
            {
                m_version += buffer[i];
            }
            else
            {
                if (m_version.substr(0, 5) != "HTTP/")
                    return galay::common::ProtoJudgeType::kProtoIllegal;
                m_version = m_version.substr(m_version.find('/') + 1);
                status = HttpHeadStatus::kHttpKey;
                ++i;
            }
        }
        break;
        case kHttpKey:
        {
            if (buffer[i] == '\r')
            {
                ++i;
                status = HttpHeadStatus::kHttpBody;
            }
            else
            {
                if (buffer[i] != ':')
                {
                    key += std::tolower(buffer[i]);
                }
                else
                {
                    if (i + 1 < n && buffer[i + 1] == ' ')
                        ++i;
                    status = HttpHeadStatus::kHttpValue;
                }
            }
        }
        break;
        case kHttpValue:
        {
            if (buffer[i] != '\r')
            {
                value += buffer[i];
            }
            else
            {
                m_headers[key] = value;
                key.clear();
                value.clear();
                ++i;
                status = HttpHeadStatus::kHttpKey;
            }
        }
        break;
        default:
            break;
        }
    }
    m_header_len = pos + 4;
    spdlog::info("[{}:{}] [[Success] [Header Len:{} Bytes]]",__FILE__,__LINE__,m_header_len + 4);
    return galay::common::ProtoJudgeType::kProtoFinished;
}


std::string 
galay::protocol::http::Http1_1_Request::EncodePdu()
{
    std::string res = this->m_method + " ";
    std::string args;
    for (auto& [k, v] : m_arg_list)
    {
        args = args + k + '=' + v + '&';
    }
    if (!m_arg_list.empty())
    {
        args.erase(--args.end());
        res += EncodeUri(std::move(this->m_uri + '?' + args));
    }
    else
    {
        res += EncodeUri(std::move(this->m_uri));
    }
    res = res + " HTTP/" + this->m_version + "\r\n";
    for (auto& [k, v] : m_headers)
    {
        res = res + k + ": " + v + "\r\n";
    }
    if (!m_headers.contains("content-length")&& !m_headers.contains("Content-Length"))
    {
        res = res + "Content-Length: " + std::to_string(this->m_body.length()) + "\r\n";
    }
    res += "\r\n";
    if (this->m_body.length() != 0)
    {
        res.append(std::move(this->m_body));
    }
    res.append("\r\n\r\n");
    return std::move(res);
}

void
galay::protocol::http::Http1_1_Request::Clear()
{
    this->m_uri.clear();
    this->m_arg_list.clear();
    this->m_body.clear();
    this->m_header_len = 0;
    this->m_headers.clear();
    this->m_method.clear();
    this->m_version.clear();
    this->m_target.clear();
}

int 
galay::protocol::http::Http1_1_Request::ConvertUri(std::string aurl)
{
    std::string uri = ConvertUri(std::move(aurl), false);
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
                    this->m_arg_list[key] = value;
                    key = "", value = "";
                    status = 0;
                }
                else
                {
                    value += args[i];
                    if (i == args.length() - 1)
                    {
                        this->m_arg_list[key] = value;
                    }
                }
            }
        }
        return -1;
    }
    this->m_uri = uri;
    return 0;
}

std::string galay::protocol::http::Http1_1_Request::EncodeUri(const std::string& s)
{
    std::string result;
    result.reserve(s.size());
    for (size_t i = 0; s[i]; i++)
    {
        switch (s[i])
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
            auto c = static_cast<uint8_t>(s[i]);
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
                result += s[i];
            }
            break;
        }
    }

    return std::move(result);
}

std::string galay::protocol::http::Http1_1_Request::ConvertUri(const std::string& s, bool convert_plus_to_space)
{
    std::string result;

    for (size_t i = 0; i < s.size(); i++)
    {
        if (s[i] == '%' && i + 1 < s.size())
        {
            if (s[i + 1] == 'u')
            {
                auto val = 0;
                if (FromHexToI(s, i + 2, 4, val))
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
                    result += s[i];
                }
            }
            else
            {
                auto val = 0;
                if (FromHexToI(s, i + 1, 2, val))
                {
                    result += static_cast<char>(val);
                    i += 2;
                }
                else
                {
                    result += s[i];
                }
            }
        }
        else if (convert_plus_to_space && s[i] == '+')
        {
            result += ' ';
        }
        else
        {
            result += s[i];
        }
    }

    return result;
}

bool galay::protocol::http::Http1_1_Request::IsHex(char c, int& v)
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

size_t galay::protocol::http::Http1_1_Request::ToUtf8(int code, char* buff)
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

bool galay::protocol::http::Http1_1_Request::FromHexToI(const std::string& s, size_t i, size_t cnt, int& val)
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
galay::protocol::http::Http1_1_Response::GetStatus()
{
    return this->m_status;
}

void 
galay::protocol::http::Http1_1_Response::SetStatus(int status)
{
    this->m_status = status;
}

std::string galay::protocol::http::Http1_1_Response::EncodePdu()
{
    std::string res = "HTTP/";
    res = res + this->m_version + ' ' + std::to_string(this->m_status) + ' ' + StatusMessage(this->m_status) + "\r\n";
    for (auto& [k, v] : this->m_headers)
    {
        res = res + k + ": " + v + "\r\n";
    }
    if (!this->m_headers.contains("Content-Length")&&!this->m_headers.contains("content-length")) {
        res = res + "Content-Length: " + std::to_string(this->m_body.length()) + "\r\n";
    }
    res += "\r\n";
    res.append(this->m_body);
    res += "\r\n\r\n";
    return std::move(res);
}


galay::common::ProtoJudgeType 
galay::protocol::http::Http1_1_Response::DecodePdu(std::string& buffer)
{
    size_t n = buffer.length();
    if(m_header_len == 0){
        galay::common::ProtoJudgeType type = ParseHead(buffer);
        if(type != galay::common::ProtoJudgeType::kProtoFinished) return type;
        spdlog::info("[{}:{}] [Head Package:{}]",__FILE__,__LINE__, buffer.substr(0,m_header_len));
        buffer.erase(0,m_header_len);
        n = buffer.length();
    }
    if(m_headers.contains("content-length")){
        size_t length = std::stoul(m_headers["content-length"]);
        if(length <= n) {
            m_body = buffer.substr(0,length);
            int invaild_len = 0;
            for (int k = length; k < n - 1; ++k)
            {
                if (isalpha(buffer[k + 1]))
                    break;
                else
                    ++invaild_len;
            }
            size_t res = length + invaild_len;
            spdlog::debug("[{}:{}] [[body is completed] [Body Len:{} Bytes] [Body Msg:{}]", __FILE__, __LINE__,length,this->m_body);
            buffer.erase(0,res);
            return galay::common::ProtoJudgeType::kProtoFinished;
        }else{
            spdlog::warn("[{}:{}] [[body is incomplete] [Body Len:{} Bytes]]",__FILE__,__LINE__,n);
            return galay::common::ProtoJudgeType::kProtoIncomplete;
        }
    }else if(m_headers.contains("transfer-encoding") && m_headers["transfer-encoding"] == "chunked")
    {
        while (!buffer.empty())
        {
            int pos = buffer.find_first_of("\r\n");
            std::string temp = buffer.substr(0,pos);
            int length;
            try
            {
                length = std::stoi(temp);
            }
            catch (const std::exception &e)
            {
                buffer.clear();
                spdlog::error("[{}:{}] [Chunck is Illegal] [ErrMsg:{}]", __FILE__, __LINE__, e.what());
                return galay::common::ProtoJudgeType::kProtoIllegal;
            }
            if(length == 0){
                buffer.erase(0,pos + 4);
                spdlog::debug("[{}:{}] [[Chunck is finished] [Chunck Len:{} Bytes]]",__FILE__,__LINE__,pos+4);
                return galay::common::ProtoJudgeType::kProtoFinished;
            }else if(length + 4 + pos > buffer.length()){
                spdlog::debug("[{}:{}] [[Chunck is incomplete] [Chunck Len:{} Bytes] [Buffer Len:{} Bytes]]",__FILE__,__LINE__,length + pos + 4,buffer.length());
                return galay::common::ProtoJudgeType::kProtoIncomplete;
            }
            this->m_body += buffer.substr(pos+2,length);
            buffer.erase(0,pos + 4 + length);
        }
    }
    return galay::common::ProtoJudgeType::kProtoFinished;
}

galay::common::ProtoJudgeType 
galay::protocol::http::Http1_1_Response::ParseHead(const std::string &buffer)
{
    if(buffer.find("\r\n\r\n") == std::string::npos) {
        if(buffer.length() > HTTP_HEADER_MAX_LEN) {
            spdlog::error("[{}:{}] [error: 'header is too long']",__FILE__,__LINE__);
            return galay::common::ProtoJudgeType::kProtoIllegal;
        }
        spdlog::warn("[{}:{}] [warn: 'header is incomplete']",__FILE__,__LINE__);
        return galay::common::ProtoJudgeType::kProtoIncomplete;
    }
    size_t n = buffer.length();
    HttpHeadStatus status = HttpHeadStatus::kHttpVersion;
    std::string status_code;
    std::string key,value;
    if (m_header_len == 0)
    {
        size_t i;
        for (i = 0; i < n; ++i)
        {
            if (status == HttpHeadStatus::kHttpBody)
                break;
            switch (status)
            {
            case kHttpVersion:
            {
                if (buffer[i] != ' ')
                {
                    m_version += buffer[i];
                }
                else
                {
                    if (m_version.substr(0, 5) != "HTTP/"){
                        spdlog::error("[{}:{}] [error: 'http version is illegal',buffer len:{}]",__FILE__,__LINE__,n);
                        Clear();
                        return galay::common::ProtoJudgeType::kProtoIllegal;
                    }
                    m_version = m_version.substr(m_version.find('/') + 1);
                    status = HttpHeadStatus::kHttpStatusCode;
                }
            }
            break;
            case kHttpStatusCode:
            {
                if (buffer[i] != ' ')
                {
                    status_code += buffer[i];
                }
                else
                {
                    try
                    {
                        m_status = std::stoi(status_code);
                    }
                    catch (std::invalid_argument &e)
                    {
                        spdlog::error("[{}:{}] [error: 'http status code is illegal',buffer len:{}]",__FILE__,__LINE__,n);
                        Clear();
                        return galay::common::ProtoJudgeType::kProtoIllegal;
                    }
                    status = HttpHeadStatus::kHttpStatusMsg;
                }
            }
            break;
            case kHttpStatusMsg:
            {
                if (buffer[i] == '\r')
                {
                    status = HttpHeadStatus::kHttpKey;
                    ++i;
                }
            }
            break;
            case kHttpKey:
            {
                if (buffer[i] == '\r')
                {
                    ++i;
                    status = HttpHeadStatus::kHttpBody;
                }
                else
                {
                    if (buffer[i] != ':')
                    {
                        key += std::tolower(buffer[i]);
                    }
                    else
                    {
                        if (i + 1 < n && buffer[i + 1] == ' ')
                            ++i;
                        status = HttpHeadStatus::kHttpValue;
                    }
                }
            }
            break;
            case kHttpValue:
            {
                if (buffer[i] != '\r')
                {
                    value += buffer[i];
                }
                else
                {
                    m_headers[key] = value;
                    key.clear();
                    value.clear();
                    ++i;
                    status = HttpHeadStatus::kHttpKey;
                }
            }
            break;
            default:
                break;
            }
        }
        m_header_len = i;
    }
    if(status != HttpHeadStatus::kHttpBody) {
        spdlog::warn("[{}:{}] [warn: 'head is incomplete',buffer len:{}]",__FILE__,__LINE__,n);
        Clear();
        return galay::common::ProtoJudgeType::kProtoIncomplete;
    }
    return galay::common::ProtoJudgeType::kProtoFinished;
}


void 
galay::protocol::http::Http1_1_Response::Clear()
{
    this->m_body.clear();
    this->m_header_len = 0;
    this->m_headers.clear();
    this->m_status = 0;
    this->m_version.clear();
}

const char* 
galay::protocol::http::Http1_1_Response::StatusMessage(int status)
{
    switch (status)
    {
    case HttpStatus::Continue_100:
        return "Continue";
    case HttpStatus::SwitchingProtocol_101:
        return "Switching Protocol";
    case HttpStatus::Processing_102:
        return "Processing";
    case HttpStatus::EarlyHints_103:
        return "Early Hints";
    case HttpStatus::OK_200:
        return "OK";
    case HttpStatus::Created_201:
        return "Created";
    case HttpStatus::Accepted_202:
        return "Accepted";
    case HttpStatus::NonAuthoritativeInformation_203:
        return "Non-Authoritative Information";
    case HttpStatus::NoContent_204:
        return "No Content";
    case HttpStatus::ResetContent_205:
        return "Reset Content";
    case HttpStatus::PartialContent_206:
        return "Partial Content";
    case HttpStatus::MultiStatus_207:
        return "Multi-Status";
    case HttpStatus::AlreadyReported_208:
        return "Already Reported";
    case HttpStatus::IMUsed_226:
        return "IM Used";
    case HttpStatus::MultipleChoices_300:
        return "Multiple Choices";
    case HttpStatus::MovedPermanently_301:
        return "Moved Permanently";
    case HttpStatus::Found_302:
        return "Found";
    case HttpStatus::SeeOther_303:
        return "See Other";
    case HttpStatus::NotModified_304:
        return "Not Modified";
    case HttpStatus::UseProxy_305:
        return "Use Proxy";
    case HttpStatus::unused_306:
        return "unused";
    case HttpStatus::TemporaryRedirect_307:
        return "Temporary Redirect";
    case HttpStatus::PermanentRedirect_308:
        return "Permanent Redirect";
    case HttpStatus::BadRequest_400:
        return "Bad Request";
    case HttpStatus::Unauthorized_401:
        return "Unauthorized";
    case HttpStatus::PaymentRequired_402:
        return "Payment Required";
    case HttpStatus::Forbidden_403:
        return "Forbidden";
    case HttpStatus::NotFound_404:
        return "Not Found";
    case HttpStatus::MethodNotAllowed_405:
        return "Method Not Allowed";
    case HttpStatus::NotAcceptable_406:
        return "Not Acceptable";
    case HttpStatus::ProxyAuthenticationRequired_407:
        return "Proxy Authentication Required";
    case HttpStatus::RequestTimeout_408:
        return "Request Timeout";
    case HttpStatus::Conflict_409:
        return "Conflict";
    case HttpStatus::Gone_410:
        return "Gone";
    case HttpStatus::LengthRequired_411:
        return "Length Required";
    case HttpStatus::PreconditionFailed_412:
        return "Precondition Failed";
    case HttpStatus::PayloadTooLarge_413:
        return "Payload Too Large";
    case HttpStatus::UriTooLong_414:
        return "URI Too Long";
    case HttpStatus::UnsupportedMediaType_415:
        return "Unsupported Media Type";
    case HttpStatus::RangeNotSatisfiable_416:
        return "Range Not Satisfiable";
    case HttpStatus::ExpectationFailed_417:
        return "Expectation Failed";
    case HttpStatus::ImATeapot_418:
        return "I'm a teapot";
    case HttpStatus::MisdirectedRequest_421:
        return "Misdirected Request";
    case HttpStatus::UnprocessableContent_422:
        return "Unprocessable Content";
    case HttpStatus::Locked_423:
        return "Locked";
    case HttpStatus::FailedDependency_424:
        return "Failed Dependency";
    case HttpStatus::TooEarly_425:
        return "Too Early";
    case HttpStatus::UpgradeRequired_426:
        return "Upgrade Required";
    case HttpStatus::PreconditionRequired_428:
        return "Precondition Required";
    case HttpStatus::TooManyRequests_429:
        return "Too Many Requests";
    case HttpStatus::RequestHeaderFieldsTooLarge_431:
        return "Request Header Fields Too Large";
    case HttpStatus::UnavailableForLegalReasons_451:
        return "Unavailable For Legal Reasons";
    case HttpStatus::NotImplemented_501:
        return "Not Implemented";
    case HttpStatus::BadGateway_502:
        return "Bad Gateway";
    case HttpStatus::ServiceUnavailable_503:
        return "Service Unavailable";
    case HttpStatus::GatewayTimeout_504:
        return "Gateway Timeout";
    case HttpStatus::HttpVersionNotSupported_505:
        return "HTTP Version Not Supported";
    case HttpStatus::VariantAlsoNegotiates_506:
        return "Variant Also Negotiates";
    case HttpStatus::InsufficientStorage_507:
        return "Insufficient Storage";
    case HttpStatus::LoopDetected_508:
        return "Loop Detected";
    case HttpStatus::NotExtended_510:
        return "Not Extended";
    case HttpStatus::NetworkAuthenticationRequired_511:
        return "Network Authentication Required";

    default:
    case HttpStatus::InternalServerError_500:
        return "Internal Server Error";
    }
    return "";
}