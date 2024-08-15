#ifndef GALAY_HTTP_1_1_H
#define GALAY_HTTP_1_1_H

#include "basic_protocol.h"
#include <vector>
#include <assert.h>
#include <algorithm>
#include <unordered_set>
#include <string_view>
#include <map>


namespace galay
{
    namespace protocol
    {
        namespace http
        {
            #define HTTP_HEADER_MAX_LEN         4096    // 头部最大长度4k
            #define HTTP_URI_MAX_LEN            2048    // uri最大长度2k
            enum HttpProStatus
            {
                kHttpHeader,
                kHttpChunck,
                kHttpBody,
                kWebsocket,
            };

            enum HttpHeadStatus
            {
                kHttpHeadMethod,
                kHttpHeadUri,
                kHttpHeadVersion,
                kHttpHeadKey,
                kHttpHeadValue,
                kHttpHeadStatusCode,
                kHttpHeadStatusMsg,
                kHttpHeadEnd,
            };

            enum HttpResponseStatus
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

            extern std::unordered_set<std::string> m_stdMethods;

            class HttpRequestHeader
            {
            public:
                using ptr = std::shared_ptr<HttpRequestHeader>;
                HttpRequestHeader() = default;
                std::string& Method();
                std::string& Uri();
                std::string& Version();
                std::map<std::string,std::string>& Args();
                std::map<std::string,std::string>& Headers();
                std::string ToString();
                bool FromString(std::string_view str);
                void CopyFrom(HttpRequestHeader::ptr header);
            private:
                void ParseArgs(std::string uri);
                std::string ConvertFromUri(std::string&& url, bool convert_plus_to_space);
                std::string ConvertToUri(std::string&& url);
                bool IsHex(char c, int &v);
                size_t ToUtf8(int code, char *buff);
                bool FromHexToI(const std::string &s, size_t i, size_t cnt, int &val);
            private:
                std::string m_method;
                std::string m_uri;                                          // uri
                std::string m_version;                                      // 版本号
                std::map<std::string, std::string> m_argList;     // 参数
                std::map<std::string, std::string> m_headers;     // 字段
            };

            class HttpRequest : public GY_Request, public galay::common::GY_DynamicCreator<GY_Request,HttpRequest>
            {
            public:
                using ptr = std::shared_ptr<HttpRequest>;
                using wptr = std::weak_ptr<HttpRequest>;
                using uptr = std::unique_ptr<HttpRequest>;
                HttpRequestHeader::ptr Header();
                std::string& Body();
                galay::protocol::ProtoJudgeType DecodePdu(std::string &buffer) override;
                std::string EncodePdu() override;
                //chunck
                bool StartChunck();
                std::string ChunckStream(std::string&& buffer);
                std::string EndChunck();
            private:
                galay::protocol::ProtoJudgeType GetHttpBody(std::string& buffer);
                galay::protocol::ProtoJudgeType GetChunckBody(std::string& buffer);
            private:
                HttpProStatus m_status = kHttpHeader;
                HttpRequestHeader::ptr m_header;
                std::string m_body;
            };

            class HttpResponseHeader
            {
            public:
                using ptr = std::shared_ptr<HttpResponseHeader>;
                std::string& Version();
                int& Code();
                std::map<std::string, std::string>& Headers();
                std::string ToString();
                bool FromString(std::string_view str);
            private:
                std::string CodeMsg(int status);
            private:
                std::string m_version;
                int m_code;
                std::map<std::string, std::string> m_headers;
            };

            class HttpResponse : public GY_Response, public galay::common::GY_DynamicCreator<GY_Response,HttpResponse>
            {
            public:
                using ptr = std::shared_ptr<HttpResponse>;
                using wptr = std::weak_ptr<HttpResponse>;
                using uptr = std::weak_ptr<HttpResponse>;
                
                HttpResponseHeader::ptr Header();
                std::string& Body();
                std::string EncodePdu() override;
                galay::protocol::ProtoJudgeType DecodePdu(std::string &buffer) override;
                //chunck
                bool StartChunck();
                std::string ChunckStream(std::string&& buffer);
                std::string EndChunck();
            private:
                galay::protocol::ProtoJudgeType GetHttpBody(std::string& buffer);
                galay::protocol::ProtoJudgeType GetChunckBody(std::string& buffer);
            private:
                HttpProStatus m_status = kHttpHeader;
                HttpResponseHeader::ptr m_header;
                std::string m_body;
            };


            extern HttpRequest::ptr DefaultHttpRequest();
        }
    }

}

#endif