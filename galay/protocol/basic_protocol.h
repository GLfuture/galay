#ifndef GALAY_BASIC_PROTOCOL_H
#define GALAY_BASIC_PROTOCOL_H

#include <string>
#include <memory>
#include "../common/reflection.h"

namespace galay
{
    namespace protocol
    {
        enum ProtoJudgeType
        {
            kProtoFinished,
            kProtoIncomplete,
            kProtoIllegal
        };

        class GY_SRequest{
        public:
            using ptr = std::shared_ptr<GY_SRequest>;
            using wptr = std::weak_ptr<GY_SRequest>;
            using uptr = std::unique_ptr<GY_SRequest>;
            virtual ProtoJudgeType DecodePdu(std::string &buffer) = 0;
        };

        class GY_CResponse{
        public:
            using ptr = std::shared_ptr<GY_CResponse>;
            using wptr = std::weak_ptr<GY_CResponse>;
            using uptr = std::unique_ptr<GY_CResponse>;
            virtual ProtoJudgeType DecodePdu(std::string &buffer) = 0;
        };

        class GY_SResponse{
        public:
            using ptr = std::shared_ptr<GY_SResponse>;
            using wptr = std::weak_ptr<GY_SResponse>;
            using uptr = std::unique_ptr<GY_SResponse>;
            virtual std::string EncodePdu() = 0;
        };
        
        class GY_CRequest{
        public:
            using ptr = std::shared_ptr<GY_CRequest>;
            using wptr = std::weak_ptr<GY_CRequest>;
            using uptr = std::unique_ptr<GY_CRequest>;
            virtual std::string EncodePdu() = 0;
        };

        //http
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
        }
        
    }
}

#endif