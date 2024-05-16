#ifndef GALAY_HTTP_1_1_H
#define GALAY_HTTP_1_1_H

#include "basic_protocol.h"
#include <vector>
#include <unordered_map>
#include <assert.h>
#include <algorithm>
#include <unordered_set>

namespace galay
{
    namespace protocol
    {
        namespace http
        {
            #define HTTP_HEADER_MAX_LEN         4096 // 头部最大长度4k
            #define HTTP_URI_MAX_LEN            2048    // uri最大长度2k
            class Http1_1_Protocol
            {
            protected:
                enum HttpHeadStatus
                {
                    HTTP_METHOD,
                    HTTP_URI,
                    HTTP_VERSION,
                    HTTP_KEY,
                    HTTP_VALUE,
                    HTTP_BODY,
                    HTTP_STATUS_CODE,
                    HTTP_STATUS_MSG,
                };

            public:
                using Ptr = ::std::shared_ptr<Http1_1_Protocol>;
                ::std::string GetVersion();
                void SetVersion(std::string &&version);
                ::std::string GetBody();
                void SetBody(std::string &&body);
                ::std::string GetHeadValue(const ::std::string &key);
                void SetHeadPair(::std::pair<::std::string, ::std::string> &&p_head);
                void SetHeaders(const std::unordered_map<std::string,std::string>& headers);
                const std::unordered_map<std::string,std::string>& GetHeaders();
            protected:
                ::std::string m_version;                                      // 版本号
                ::std::unordered_map<::std::string, ::std::string> m_headers; // 字段
                ::std::string m_body = "";                                    // body
            };

            class Http1_1_Request : public Http1_1_Protocol, public GY_TcpRequest, public GY_TcpResponse
            {
            private:
                static ::std::unordered_set<::std::string> m_std_methods;

            public:
                using ptr = ::std::shared_ptr<Http1_1_Request>;
                ::std::string GetArgValue(const ::std::string &key);
                void SetArgPair(::std::pair<::std::string, ::std::string> &&p_arg);
                ::std::string GetMethod();
                void SetMethod(std::string &&method);
                // for server
                ::std::string GetUri();
                void SetUri(std::string &&uri);
                ProtoJudgeType DecodePdu(::std::string &buffer) override;
                // for  client
                ::std::string EncodePdu() override;

                virtual void Clear() override;
            private:
                ProtoJudgeType ParseHead(const ::std::string &buffer);
            private:
                int ConvertUri(::std::string aurl);
                ::std::string EncodeUri(const ::std::string &s);
                ::std::string ConvertUri(const ::std::string &s, bool convert_plus_to_space);
                bool IsHex(char c, int &v);
                size_t ToUtf8(int code, char *buff);
                bool FromHexToI(const ::std::string &s, size_t i, size_t cnt, int &val);

            private:
                uint32_t m_header_len = 0;                                     // 头长度
                ::std::string m_method;                                        // http协议类型
                ::std::string m_target;                                        //?后的内容
                ::std::string m_uri;                                           // uri
                ::std::unordered_map<::std::string, ::std::string> m_arg_list; // 参数
            };

            class Http1_1_Response : public Http1_1_Protocol, public GY_TcpResponse, public GY_TcpRequest
            {
            public:
                using ptr = ::std::shared_ptr<Http1_1_Response>;
                // look for httplib's status https://github.com/yhirose/cpp-httplib/blob/master/httplib.h
                enum http_protocol_status
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
                    unused_306 = 306,
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

                int GetStatus();
                void SetStatus(int status);

                ::std::string EncodePdu() override;

                ProtoJudgeType DecodePdu(::std::string &buffer) override;

                virtual void Clear() override;
                // look for httplib's status https://github.com/yhirose/cpp-httplib/blob/master/httplib.h
                const char *StatusMessage(int status);
            private:
                ProtoJudgeType ParseHead(const ::std::string &buffer);
            private:
                int m_header_len = 0;
                int m_status = 0;
            };
        }
    }

}

#endif