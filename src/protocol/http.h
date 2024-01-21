#ifndef GALAY_HTTP_H
#define GALAY_HTTP_H

#include "basic_protocol.h"
#include "../kernel/error.h"
#include <vector>
#include <unordered_map>
#include <assert.h>
#include <algorithm>
#include <regex>

namespace galay
{
    class Http_Protocol
    {
    public:
        using Ptr = std::shared_ptr<Http_Protocol>;
        std::string &get_version();

        std::string &get_body();

        std::string get_head_value(const std::string &key);

        void set_head_kv_pair(std::pair<std::string, std::string> &&p_head);

    protected:
        std::string m_version;                                     // 版本号
        std::unordered_map<std::string, std::string> m_filed_list; // 字段
        std::string m_body = "";                                   // body
    };

    class Http_Request : public Http_Protocol, public Request_Base, public Response_Base
    {
    public:
        using ptr = std::shared_ptr<Http_Request>;

        std::string get_arg_value(const std::string &key);
        
        void set_arg_kv_pair(std::pair<std::string, std::string> &&p_arg);

        std::string &get_method();

        // for server
        std::string &get_url_path();
        // for server
        int decode(const std::string &buffer, int &state) override;

        // for  client
        std::string encode() override;

        int proto_type() override;

        int proto_fixed_len() override;

        int proto_extra_len() override;

        void set_extra_msg(std::string&& msg) override;

    private:

        int decode_url(std::string aurl);

        std::string encode_url(const std::string &s);

        std::string decode_url(const std::string &s, bool convert_plus_to_space);

        bool is_hex(char c, int &v);

        size_t to_utf8(int code, char *buff);

        bool from_hex_to_i(const std::string &s, size_t i, size_t cnt, int &val);

    private:
        std::string m_method;                                    // http协议类型
        std::string m_target;                                    //?后的内容
        std::string m_url_path;                                  // urlpath
        std::unordered_map<std::string, std::string> m_arg_list; // 参数
    };

    class Http_Response : public Http_Protocol, public Response_Base, public Request_Base
    {
    public:
        using ptr = std::shared_ptr<Http_Response>;
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

        int &get_status();
        
        std::string encode();

        int decode(const std::string &buffer, int &state) override;

        int proto_type() override;

        int proto_fixed_len() override;

        int proto_extra_len() override;

        void set_extra_msg(std::string&& msg) override;

        // look for httplib's status https://github.com/yhirose/cpp-httplib/blob/master/httplib.h
        const char *status_message(int status);

    private:
        int m_status;
    };

}

#endif