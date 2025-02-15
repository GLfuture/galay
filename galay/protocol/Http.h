#ifndef GALAY_HTTP_H
#define GALAY_HTTP_H

#include "Protocol.h"
#include <map>
#include <unordered_set>
#include <string_view>
#include <sstream>


namespace galay::error
{

enum HttpErrorCode
{
    kHttpError_NoError = 0,
    kHttpError_HeaderInComplete,
    kHttpError_BodyInComplete,
    kHttpError_HeaderTooLong,
    kHttpError_UriTooLong,
    kHttpError_ChunckHasError,
    kHttpError_HttpCodeInvalid,
    kHttpError_HeaderPairExist,
    kHttpError_HeaderPairNotExist,
    kHttpError_BadRequest,
};

class HttpError
{
public:
    using ptr = std::shared_ptr<HttpError>;
    using wptr = std::weak_ptr<HttpError>;
    using uptr = std::unique_ptr<HttpError>;
    bool HasError() const;
    HttpErrorCode& Code();
    void Reset();
    std::string ToString(HttpErrorCode code) const;
protected:
    HttpErrorCode m_code = kHttpError_NoError;
};


}

namespace galay::http
{

#define HTTP_HEADER_MAX_LEN         4096    // 头部最大长度4k
#define HTTP_URI_MAX_LEN            2048    // uri最大长度2k

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

class HeaderPair
{
public:
    bool HasKey(const std::string& key) const;
    std::string GetValue(const std::string& key);
    error::HttpErrorCode RemoveHeaderPair(const std::string& key);
    error::HttpErrorCode AddHeaderPair(const std::string& key, const std::string& value);
    error::HttpErrorCode SetHeaderPair(const std::string& key, const std::string& value);
    std::string ToString();
    void Clear();
    HeaderPair& operator=(const HeaderPair& headerPair);
private:
    std::ostringstream m_stream;
    std::map<std::string, std::string> m_headerPairs;
};

class HttpRequestHeader
{
public:
    using ptr = std::shared_ptr<HttpRequestHeader>;
    HttpRequestHeader() = default;
    HttpMethod& Method();
    std::string& Uri();
    HttpVersion& Version();
    std::map<std::string,std::string>& Args();
    HeaderPair& HeaderPairs();
    std::string ToString();
    error::HttpErrorCode FromString(HttpDecodeStatus& status, std::string_view str, size_t& next_index);
    void CopyFrom(const HttpRequestHeader::ptr& header);
    void Reset();
private:
    void ParseArgs(const std::string& uri);
    std::string ConvertFromUri(std::string&& url, bool convert_plus_to_space);
    std::string ConvertToUri(std::string&& url);
    bool IsHex(char c, int &v);
    size_t ToUtf8(int code, char *buff);
    bool FromHexToI(const std::string &s, size_t i, size_t cnt, int &val);
private:
    std::ostringstream m_stream;
    HttpMethod m_method;
    std::string m_uri;                                          // uri
    HttpVersion m_version;                                      // 版本号
    std::map<std::string, std::string> m_argList;               // 参数
    HeaderPair m_headerPairs;                                   // 字段
};

class HttpRequest final : public Request, public galay::common::DynamicCreator<Request,HttpRequest>
{
public:
    using ptr = std::shared_ptr<HttpRequest>;
    using wptr = std::weak_ptr<HttpRequest>;
    using uptr = std::unique_ptr<HttpRequest>;
    
    HttpRequest();
    HttpRequestHeader::ptr Header();
    void SetContent(const std::string& type, std::string&& content);
    std::string_view GetContent();
    std::pair<bool,size_t> DecodePdu(const std::string_view &buffer) override;
    [[nodiscard]] std::string EncodePdu() const override;
    [[nodiscard]] bool HasError() const override;
    [[nodiscard]] int GetErrorCode() const override;
    std::string GetErrorString() override;
    void Reset() override;
    //chunk
    bool StartChunk();
    std::string ToChunkData(std::string&& buffer);
    std::string EndChunk();
private:
    bool GetHttpBody(const std::string_view& buffer);
    bool GetChunkBody(const std::string_view& buffer);
private:
    HttpDecodeStatus m_status;
    HttpRequestHeader::ptr m_header;
    size_t m_next_index;
    std::string m_body;
    error::HttpError::ptr m_error;
};

class HttpResponseHeader
{
public:
    using ptr = std::shared_ptr<HttpResponseHeader>;
    HttpVersion& Version();
    HttpStatusCode& Code();
    HeaderPair& HeaderPairs();
    std::string ToString();
    error::HttpErrorCode FromString(HttpDecodeStatus& status, std::string_view str, size_t& next_index);
private:
    std::ostringstream m_stream;
    HttpStatusCode m_code;
    HttpVersion m_version;
    HeaderPair m_headerPairs;
};

class HttpResponse : public Response, public galay::common::DynamicCreator<Response,HttpResponse>
{
public:
    using ptr = std::shared_ptr<HttpResponse>;
    using wptr = std::weak_ptr<HttpResponse>;
    using uptr = std::weak_ptr<HttpResponse>;
    HttpResponse();
    HttpResponseHeader::ptr Header();
    void SetContent(const std::string& type, std::string&& content);
    std::string_view GetContent();
    [[nodiscard]] std::string EncodePdu() const override;
    std::pair<bool,size_t> DecodePdu(const std::string_view &buffer) override;
    [[nodiscard]] bool HasError() const override;
    [[nodiscard]] int GetErrorCode() const override;
    std::string GetErrorString() override;
    void Reset() override;
    
    //chunck
    bool StartChunck();
    std::string ToChunckData(std::string&& buffer);
    std::string EndChunck();
private:
    bool GetHttpBody(const std::string_view& buffer);
    bool GetChunckBody(const std::string_view& buffer);
private:
    HttpDecodeStatus m_status;
    HttpResponseHeader::ptr m_header;
    std::string m_body;
    size_t m_next_index;
    error::HttpError::ptr m_error;
};


}

namespace galay
{

#define GET HttpMethod::Http_Method_Get
#define POST HttpMethod::Http_Method_Post
#define PUT HttpMethod::Http_Method_Put
#define DELETE HttpMethod::Http_Method_Delete
#define HEAD HttpMethod::Http_Method_Head
#define OPTIONS HttpMethod::Http_Method_Options
#define CONNECT HttpMethod::Http_Method_Connect
#define TRACE HttpMethod::Http_Method_Trace


#define HTTP_1_0 HttpVersion::Http_Version_1_0
#define HTTP_1_1 HttpVersion::Http_Version_1_1
#define HTTP_2_0 HttpVersion::Http_Version_2_0

}
    

#endif