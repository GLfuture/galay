#ifndef GALAY_HTTP_COMMON_HPP
#define GALAY_HTTP_COMMON_HPP

#include <string>
#include <unordered_map>
#include "galay/common/Base.h"
#include "galay/kernel/Log.h"

namespace galay::http 
{

#define DEFAULT_HTTP_RECV_TIME_MS                       10000
#define DEFAULT_HTTP_SEND_TIME_MS                       10000
#define DEFAULT_HTTP_MAX_HEADER_SIZE                    4096    // http头最大长度4k
#define DEFAULT_HTTP_MAX_URI_LEN                        2048    // uri最大长度2k

#define DEFAULT_HTTP_KEEPALIVE_TIME_MS                  (7500 * 1000)

#define SERVER_NAME "galay"

#define GALAY_SERVER SERVER_NAME "/" GALAY_VERSION

inline std::atomic_int32_t gHttpMaxHeaderSize = DEFAULT_HTTP_MAX_HEADER_SIZE;
inline std::atomic_int32_t gHttpMaxUriSize = DEFAULT_HTTP_MAX_URI_LEN;


//log
#define DEFAULT_LOG_METHOD_LENGTH       20
#define DEFAULT_LOG_URI_PEER_LIMIT      50
#define DEFAULT_LOG_STATUS_LENGTH       20
#define DEFAULT_LOG_STATUS_TEXT_LENGTH  50

enum class HttpDecodeStatus: int
{
    kHttpHeadMethod,
    kHttpHeadUri,
    kHttpHeadVersion,
    kHttpHeadKey,
    kHttpHeadValue,
    kHttpHeadStatusCode,
    kHttpHeadStatusMsg,
    kHttpHeadEnd,
    kHttpBody,
};

enum class HttpMethod: int
{
    Http_Method_Get = 0,
    Http_Method_Post = 1,
    Http_Method_Head = 2,
    Http_Method_Put = 3,
    Http_Method_Delete = 4,
    Http_Method_Trace = 5,
    Http_Method_Options = 6,
    Http_Method_Connect = 7,
    Http_Method_Patch = 8,
    Http_Method_Unknown,
};

enum class HttpVersion: int
{
    Http_Version_1_0,   
    Http_Version_1_1,
    Http_Version_2_0,
    Http_Version_3_0,
    Http_Version_Unknown,
};

enum class HttpStatusCode: int
{
    // Information responses
    Continue_100 = 100,
    SwitchingProtocol_101 = 101,
    Processing_102 = 102,
    EarlyHints_103 = 103,

    // Successful responses
    OK_200 = 200,
    Created_201 = 201,
    Accepted_202 = 202,
    NonAuthoritativeInformation_203 = 203,
    NoContent_204 = 204,
    ResetContent_205 = 205,
    PartialContent_206 = 206,
    MultiStatus_207 = 207,
    AlreadyReported_208 = 208,
    IMUsed_226 = 226,

    // Redirection messages
    MultipleChoices_300 = 300,
    MovedPermanently_301 = 301,
    Found_302 = 302,
    SeeOther_303 = 303,
    NotModified_304 = 304,
    UseProxy_305 = 305,
    Unused_306 = 306,
    TemporaryRedirect_307 = 307,
    PermanentRedirect_308 = 308,

    // Client error responses
    BadRequest_400 = 400,
    Unauthorized_401 = 401,
    PaymentRequired_402 = 402,
    Forbidden_403 = 403,
    NotFound_404 = 404,
    MethodNotAllowed_405 = 405,
    NotAcceptable_406 = 406,
    ProxyAuthenticationRequired_407 = 407,
    RequestTimeout_408 = 408,
    Conflict_409 = 409,
    Gone_410 = 410,
    LengthRequired_411 = 411,
    PreconditionFailed_412 = 412,
    PayloadTooLarge_413 = 413,
    UriTooLong_414 = 414,
    UnsupportedMediaType_415 = 415,
    RangeNotSatisfiable_416 = 416,
    ExpectationFailed_417 = 417,
    ImATeapot_418 = 418,
    MisdirectedRequest_421 = 421,
    UnprocessableContent_422 = 422,
    Locked_423 = 423,
    FailedDependency_424 = 424,
    TooEarly_425 = 425,
    UpgradeRequired_426 = 426,
    PreconditionRequired_428 = 428,
    TooManyRequests_429 = 429,
    RequestHeaderFieldsTooLarge_431 = 431,
    UnavailableForLegalReasons_451 = 451,

    // Server error responses
    InternalServerError_500 = 500,
    NotImplemented_501 = 501,
    BadGateway_502 = 502,
    ServiceUnavailable_503 = 503,
    GatewayTimeout_504 = 504,
    HttpVersionNotSupported_505 = 505,
    VariantAlsoNegotiates_506 = 506,
    InsufficientStorage_507 = 507,
    LoopDetected_508 = 508,
    NotExtended_510 = 510,
    NetworkAuthenticationRequired_511 = 511,
};

extern std::string HttpVersionToString(HttpVersion version);
extern HttpVersion StringToHttpVersion(std::string_view str);
extern std::string HttpMethodToString(HttpMethod method);
extern HttpMethod StringToHttpMethod(std::string_view str);
extern std::string HttpStatusCodeToString(HttpStatusCode code); 

class MimeType
{
public:
    static std::string ConvertToMimeType(const std::string& type);
private:
    static std::unordered_map<std::string, std::string> mimeTypeMap;
};


class HttpLogger
{
public:
    using uptr = std::unique_ptr<HttpLogger>;

    HttpLogger();

    static HttpLogger* GetInstance();

    Logger* GetLogger() {
        return m_logger.get();
    }

    void ResetLogger(std::unique_ptr<Logger> logger) {
        m_logger = std::move(logger);
    }
private:
    static HttpLogger::uptr m_instance;
    std::unique_ptr<Logger> m_logger;
    std::shared_ptr<spdlog::details::thread_pool> m_thread_pool;
};

inline HttpLogger::uptr HttpLogger::m_instance = nullptr;

const spdlog::string_view_t RESET_COLOR = "\033[0m";
const spdlog::string_view_t GRAY_COLOR = "\033[37m";

inline spdlog::string_view_t status_color(HttpStatusCode status_code) {
    using enum HttpStatusCode;
    int status = static_cast<int>(status_code);
    if (status >= 100 && status < 200) { // 1xx Informational
        return "\033[36m";  // 青色
    } else if (status >= 200 && status < 300) { // 2xx Success
        return "\033[32m";  // 绿色
    } else if (status >= 300 && status < 400) { // 3xx Redirection
        return "\033[33m";  // 黄色
    } else if (status >= 400 && status < 500) { // 4xx Client Error
        return "\033[31m";  // 红色
    } else if (status >= 500) { // 5xx Server Error
        return "\033[35m";  // 品红
    }
    return "\033[90m"; // 未知状态码用暗灰色
}

inline spdlog::string_view_t method_color(HttpMethod method) {
    using enum HttpMethod; 

    switch (method) {
    case Http_Method_Get:     return "\033[32m"; // 绿色 - 安全操作
    case Http_Method_Post:    return "\033[33m"; // 黄色 - 数据修改
    case Http_Method_Put:     return "\033[34m"; // 蓝色 - 更新操作  
    case Http_Method_Delete:  return "\033[31m"; // 红色 - 危险操作
    case Http_Method_Head:    return "\033[36m"; // 青色 - 元数据操作
    case Http_Method_Options: return "\033[35m"; // 品红 - 调试用途
    case Http_Method_Patch:   return "\033[35;1m"; // 亮品红 - 部分更新
    case Http_Method_Trace:   return "\033[37m"; // 灰色 - 诊断用途
    case Http_Method_Connect: return "\033[33;1m"; // 亮黄色 - 隧道连接
    case Http_Method_Unknown: 
    default:                  return "\033[90m"; // 暗灰色 - 未知方法
    }
    return "\033[0m";
}

inline spdlog::string_view_t resp_time_color(size_t ms) {
    if (ms < 100) return "\033[32m";      // 绿色：优秀性能
    if (ms < 500) return "\033[33m";     // 黄色：需关注
    return "\033[31m";                   // 红色：严重延迟
}

inline int method_length(HttpMethod method)
{
    return DEFAULT_LOG_METHOD_LENGTH;
}

inline int uri_length(const std::string& uri) 
{
    int length = uri.length() + 2;
    return (length / DEFAULT_LOG_URI_PEER_LIMIT + 1) * DEFAULT_LOG_URI_PEER_LIMIT; 
}

inline int status_length(HttpStatusCode code)
{
    return DEFAULT_LOG_STATUS_LENGTH;
}

inline int status_code_length(HttpStatusCode code)
{
    return DEFAULT_LOG_STATUS_TEXT_LENGTH;
}

#define REQUEST_LOG(METHOD, URI, REMOTE) {\
    std::string method = fmt::format("[{}{}{}]", method_color(METHOD), HttpMethodToString(METHOD), RESET_COLOR);\
    std::string uri = fmt::format("[{}{}{}]", method_color(METHOD), URI, RESET_COLOR);\
    HttpLogger::GetInstance()->GetLogger()->SpdLogger()->info( \
    "{:<{}} {:<{}} [{}Remote: {}{}]", \
    method, method_length(METHOD), \
    uri, uri_length(URI), \
    "\033[38;5;39m", REMOTE, RESET_COLOR); }

#define RESPONSE_LOG(STATUS, DURING_MS)   {\
    std::string status = fmt::format("[{}{}{}]", status_color(STATUS), std::to_string(static_cast<int>(STATUS)), RESET_COLOR);\
    std::string status_text = fmt::format("[{}{}{}]", status_color(STATUS), HttpStatusCodeToString(STATUS), RESET_COLOR);\
    HttpLogger::GetInstance()->GetLogger()->SpdLogger()->info( \
    "{:<{}} {:<{}} [{}During: {}ms{}]", \
    status, status_length(STATUS),\
    status_text, status_code_length(STATUS), \
    resp_time_color(DURING_MS), std::to_string(DURING_MS), RESET_COLOR); }
}



#endif