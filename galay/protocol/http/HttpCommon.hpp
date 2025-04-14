#ifndef GALAY_HTTP_COMMON_HPP
#define GALAY_HTTP_COMMON_HPP

#include <string>
#include <unordered_map>

namespace galay::http 
{

#define DEFAULT_HTTP_RECV_TIME_MS                       10000
#define DEFAULT_HTTP_SEND_TIME_MS                       10000
#define DEFAULT_HTTP_MAX_HEADER_SIZE                    4096

#define DEFAULT_LIBAIO_MAX_EVENT                        1024

#define HTTP_HEADER_MAX_LEN         4096    // 头部最大长度4k
#define HTTP_URI_MAX_LEN            2048    // uri最大长度2k

#define DEFAULT_HTTP_KEEPALIVE_TIME_MS              (7500 * 1000)

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
    Http_Method_Post,
    Http_Method_Head,
    Http_Method_Put,
    Http_Method_Delete,
    Http_Method_Trace,
    Http_Method_Options,
    Http_Method_Connect,
    Http_Method_Patch,
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
    
}

#endif