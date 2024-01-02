#include "http.h"

std::string &galay::Http_Protocol::get_version()
{
    return this->m_version;
}

std::string &galay::Http_Protocol::get_body()
{
    return this->m_body;
}

std::string galay::Http_Protocol::get_head_value(const std::string &key)
{
    auto it = this->m_filed_list.find(key);
    if (it == this->m_filed_list.end())
        return "";
    return it->second;
}

void galay::Http_Protocol::set_head_kv_pair(std::pair<std::string, std::string> &&p_head)
{
    this->m_filed_list[p_head.first] = p_head.second;
}

std::string galay::Http_Request::get_arg_value(const std::string &key)
{
    auto it = this->m_arg_list.find(key);
    if (it == this->m_arg_list.end())
        return "";
    return it->second;
}

void galay::Http_Request::set_arg_kv_pair(std::pair<std::string, std::string> &&p_arg)
{
    this->m_arg_list[p_arg.first] = p_arg.second;
}

std::string &galay::Http_Request::get_method()
{
    return this->m_method;
}

std::string &galay::Http_Request::get_url_path()
{
    return this->m_url_path;
}

int galay::Http_Request::decode(const std::string &buffer, int &state)
{
    state = error::base_error::GY_SUCCESS;
    int beg = 0;
    int end = buffer.find("\r\n\r\n");
    if (end == std::string::npos)
    {
        state = error::protocol_error::GY_PROTOCOL_INCOMPLETE;
        return -1;
    }
    std::string head = buffer.substr(beg, end);
    int hbeg = 0, hend = 0;
    std::vector<std::string> lines;
    do
    {
        hend = head.find("\r\n", hbeg);
        std::string temp = head.substr(hbeg, hend - hbeg);
        if (!temp.empty())
            lines.emplace_back(temp);
        hbeg = hend + 2;
    } while (hend != std::string::npos);
    for (int i = 0; i < lines.size(); i++)
    {
        int index = 0;
        if (i == 0)
        {
            index = decode_method(lines[i], index);
            index = decode_url(lines[i], index);
            index = decode_version(lines[i], index);
        }
        else
        {
            int tbeg = 0, tend = 0;
            tend = lines[i].find_first_of(":");
            std::string key = lines[i].substr(0, tend);
            tbeg = tend + 2;
            std::string value = lines[i].substr(tbeg);
            this->m_filed_list[key] = value;
        }
    }
    if (this->m_method.compare("OPTIONS") == 0)
    {
        return end + 4;
    }
    beg = end + 4;
    std::string len_str = get_head_value("Content-Length");
    if (!len_str.empty())
    {
        int content_len = atoi(len_str.c_str());
        std::string other = buffer.substr(beg);
        if (content_len > other.length())
        {
            state = error::protocol_error::GY_PROTOCOL_INCOMPLETE;
            return -1;
        }
        this->m_body = buffer.substr(beg, content_len);
        return beg + content_len + 2;
    }
    if (beg >= buffer.length() - 1)
        return beg;
    end = buffer.find("\r\n\r\n", beg);
    if (end == std::string::npos)
    {
        state = error::protocol_error::GY_PROTOCOL_BAD_REQUEST;
        return -1;
    }
    this->m_body = buffer.substr(beg, end - beg);
    return end + 4;
}

std::string galay::Http_Request::encode()
{
    std::string res = this->m_method + " ";
    std::string args;
    for (auto &[k, v] : m_arg_list)
    {
        args = args + k + '=' + v + '&';
    }
    if (!m_arg_list.empty())
    {
        args.erase(--args.end());
        res += encode_url(std::move(this->m_url_path + '?' + args));
    }
    else
    {
        res += encode_url(std::move(this->m_url_path));
    }
    res = res + ' ' + this->m_version + "\r\n";
    for (auto &[k, v] : m_filed_list)
    {
        res = res + k + ": " + v + "\r\n";
    }
    if (m_filed_list.find("Content-Length") == m_filed_list.end())
    {
        res = res + "Content-Length: " + std::to_string(this->m_body.length());
    }
    res += "\r\n";
    if (this->m_body.length() != 0)
    {
        res.append(std::move(this->m_body));
    }
    res.append(std::move("\r\n\r\n"));
    return res;
}

int galay::Http_Request::decode_version(std::string str, int index)
{
    int end = str.find("\r\n");
    this->m_version = str.substr(index, end - index);
    return end + 1;
}

int galay::Http_Request::decode_method(std::string str, int index)
{
    int end = str.find_first_of(' ');
    this->m_method = str.substr(index, end);
    return end + 1;
}

int galay::Http_Request::decode_url(std::string str, int index)
{
    int end = str.find_first_of(' ', index);
    std::string temp = str.substr(index, end - index);
    std::string url = decode_url(std::move(temp), false);
    int argindx = url.find('?');
    if (argindx != std::string::npos)
    {
        int cur = 0;
        this->m_url_path = url.substr(cur, argindx - cur);
        std::string args = url.substr(argindx + 1);
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
        return end + 1;
    }
    this->m_url_path = url;
    return end + 1;
}

std::string galay::Http_Request::encode_url(const std::string &s)
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

    return result;
}

std::string galay::Http_Request::decode_url(const std::string &s, bool convert_plus_to_space)
{
    std::string result;

    for (size_t i = 0; i < s.size(); i++)
    {
        if (s[i] == '%' && i + 1 < s.size())
        {
            if (s[i + 1] == 'u')
            {
                auto val = 0;
                if (from_hex_to_i(s, i + 2, 4, val))
                {
                    char buff[4];
                    size_t len = to_utf8(val, buff);
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
                if (from_hex_to_i(s, i + 1, 2, val))
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

bool galay::Http_Request::is_hex(char c, int &v)
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

size_t galay::Http_Request::to_utf8(int code, char *buff)
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

bool galay::Http_Request::from_hex_to_i(const std::string &s, size_t i, size_t cnt, int &val)
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
        if (is_hex(s[i], v))
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

int &galay::Http_Response::get_status()
{
    return this->m_status;
}

std::string galay::Http_Response::encode()
{
    std::string res = this->m_version + ' ' + std::to_string(this->m_status) + ' ' + status_message(this->m_status) + "\r\n";
    for (auto &[k, v] : this->m_filed_list)
    {
        res = res + k + ": " + v + "\r\n";
    }
    res += "\r\n";
    res.append(this->m_body);
    res += "\r\n\r\n";
    return res;
}

int galay::Http_Response::decode(const std::string &buffer, int &state)
{
    int beg = 0;
    int end = buffer.find("\r\n\r\n");
    if (end == std::string::npos)
    {
        state = 1;
        return -1;
    }
    std::string head = buffer.substr(beg, end);
    int hbeg = 0, hend = 0;
    std::vector<std::string> lines;
    do
    {
        hend = head.find("\r\n", hbeg);
        std::string temp = head.substr(hbeg, hend - hbeg);
        if (!temp.empty())
            lines.emplace_back(temp);
        hbeg = hend + 2;
    } while (hend != std::string::npos);
    for (int i = 0; i < lines.size(); i++)
    {
        if (i == 0)
        {
            int indx = lines[i].find_first_of(' ');
            this->m_version = lines[i].substr(0, indx);
            std::string status = lines[i].substr(indx + 1, lines[i].find(' ', indx + 1) - indx - 1);
            this->m_status = atoi(status.c_str());
        }
        else
        {
            int tbeg = 0, tend = 0;
            tend = lines[i].find_first_of(":");
            std::string key = lines[i].substr(0, tend);
            tbeg = tend + 2;
            std::string value = lines[i].substr(tbeg);
            this->m_filed_list[key] = value;
        }
    }

    //
    beg = end + 4;
    std::string len_str = get_head_value("Content-Length");
    if (!len_str.empty())
    {
        int content_len = atoi(len_str.c_str());
        std::string other = buffer.substr(beg);
        if (content_len > other.length())
        {
            state = error::protocol_error::GY_PROTOCOL_INCOMPLETE;
            return -1;
        }
        this->m_body = buffer.substr(beg, content_len);
        return beg + content_len + 2;
    }
    if (beg >= buffer.length() - 1)
        return beg;
    end = buffer.find("\r\n\r\n", beg);
    if (end == std::string::npos)
    {
        state = error::protocol_error::GY_PROTOCOL_BAD_REQUEST;
        return -1;
    }
    this->m_body = buffer.substr(beg, end - beg);
    return end + 4;
}

const char *galay::Http_Response::status_message(int status)
{
    switch (status)
    {
    case http_protocol_status::Continue_100:
        return "Continue";
    case http_protocol_status::SwitchingProtocol_101:
        return "Switching Protocol";
    case http_protocol_status::Processing_102:
        return "Processing";
    case http_protocol_status::EarlyHints_103:
        return "Early Hints";
    case http_protocol_status::OK_200:
        return "OK";
    case http_protocol_status::Created_201:
        return "Created";
    case http_protocol_status::Accepted_202:
        return "Accepted";
    case http_protocol_status::NonAuthoritativeInformation_203:
        return "Non-Authoritative Information";
    case http_protocol_status::NoContent_204:
        return "No Content";
    case http_protocol_status::ResetContent_205:
        return "Reset Content";
    case http_protocol_status::PartialContent_206:
        return "Partial Content";
    case http_protocol_status::MultiStatus_207:
        return "Multi-Status";
    case http_protocol_status::AlreadyReported_208:
        return "Already Reported";
    case http_protocol_status::IMUsed_226:
        return "IM Used";
    case http_protocol_status::MultipleChoices_300:
        return "Multiple Choices";
    case http_protocol_status::MovedPermanently_301:
        return "Moved Permanently";
    case http_protocol_status::Found_302:
        return "Found";
    case http_protocol_status::SeeOther_303:
        return "See Other";
    case http_protocol_status::NotModified_304:
        return "Not Modified";
    case http_protocol_status::UseProxy_305:
        return "Use Proxy";
    case http_protocol_status::unused_306:
        return "unused";
    case http_protocol_status::TemporaryRedirect_307:
        return "Temporary Redirect";
    case http_protocol_status::PermanentRedirect_308:
        return "Permanent Redirect";
    case http_protocol_status::BadRequest_400:
        return "Bad Request";
    case http_protocol_status::Unauthorized_401:
        return "Unauthorized";
    case http_protocol_status::PaymentRequired_402:
        return "Payment Required";
    case http_protocol_status::Forbidden_403:
        return "Forbidden";
    case http_protocol_status::NotFound_404:
        return "Not Found";
    case http_protocol_status::MethodNotAllowed_405:
        return "Method Not Allowed";
    case http_protocol_status::NotAcceptable_406:
        return "Not Acceptable";
    case http_protocol_status::ProxyAuthenticationRequired_407:
        return "Proxy Authentication Required";
    case http_protocol_status::RequestTimeout_408:
        return "Request Timeout";
    case http_protocol_status::Conflict_409:
        return "Conflict";
    case http_protocol_status::Gone_410:
        return "Gone";
    case http_protocol_status::LengthRequired_411:
        return "Length Required";
    case http_protocol_status::PreconditionFailed_412:
        return "Precondition Failed";
    case http_protocol_status::PayloadTooLarge_413:
        return "Payload Too Large";
    case http_protocol_status::UriTooLong_414:
        return "URI Too Long";
    case http_protocol_status::UnsupportedMediaType_415:
        return "Unsupported Media Type";
    case http_protocol_status::RangeNotSatisfiable_416:
        return "Range Not Satisfiable";
    case http_protocol_status::ExpectationFailed_417:
        return "Expectation Failed";
    case http_protocol_status::ImATeapot_418:
        return "I'm a teapot";
    case http_protocol_status::MisdirectedRequest_421:
        return "Misdirected Request";
    case http_protocol_status::UnprocessableContent_422:
        return "Unprocessable Content";
    case http_protocol_status::Locked_423:
        return "Locked";
    case http_protocol_status::FailedDependency_424:
        return "Failed Dependency";
    case http_protocol_status::TooEarly_425:
        return "Too Early";
    case http_protocol_status::UpgradeRequired_426:
        return "Upgrade Required";
    case http_protocol_status::PreconditionRequired_428:
        return "Precondition Required";
    case http_protocol_status::TooManyRequests_429:
        return "Too Many Requests";
    case http_protocol_status::RequestHeaderFieldsTooLarge_431:
        return "Request Header Fields Too Large";
    case http_protocol_status::UnavailableForLegalReasons_451:
        return "Unavailable For Legal Reasons";
    case http_protocol_status::NotImplemented_501:
        return "Not Implemented";
    case http_protocol_status::BadGateway_502:
        return "Bad Gateway";
    case http_protocol_status::ServiceUnavailable_503:
        return "Service Unavailable";
    case http_protocol_status::GatewayTimeout_504:
        return "Gateway Timeout";
    case http_protocol_status::HttpVersionNotSupported_505:
        return "HTTP Version Not Supported";
    case http_protocol_status::VariantAlsoNegotiates_506:
        return "Variant Also Negotiates";
    case http_protocol_status::InsufficientStorage_507:
        return "Insufficient Storage";
    case http_protocol_status::LoopDetected_508:
        return "Loop Detected";
    case http_protocol_status::NotExtended_510:
        return "Not Extended";
    case http_protocol_status::NetworkAuthenticationRequired_511:
        return "Network Authentication Required";

    default:
    case http_protocol_status::InternalServerError_500:
        return "Internal Server Error";
    }
    return "";
}