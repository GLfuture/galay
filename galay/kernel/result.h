#ifndef GALAY_RESULT_H
#define GALAY_RESULT_H

#include <memory>
#include <any>
#include "../protocol/http.h"
#include "../protocol/smtp.h"
#include "../protocol/dns.h"

namespace galay{
    namespace coroutine
    {
        class GroupAwaiter;
        class WaitGroup;
    }

    namespace result
    {
        class NetResult
        {
        public:
            using ptr = std::shared_ptr<NetResult>;
            using wptr = std::weak_ptr<NetResult>;
            NetResult();
            void Done();
            bool Success();
            std::any Result();
            std::string Error();
            void AddTaskNum(uint16_t taskNum);
            coroutine::GroupAwaiter& Wait();
        protected:
            bool m_success;
            //tcp:一次请求(std::string) ,多次请求(std::queue<std::string>)
            //udp:struct UdpResInfo
            std::any m_result;   
            std::string m_errMsg;
            std::shared_ptr<coroutine::WaitGroup> m_waitGroup;
        };

        class NetResultInner:virtual public NetResult
        {
        public:
            using ptr = std::shared_ptr<NetResultInner>;
            using wptr = std::weak_ptr<NetResultInner>;
            void SetResult(std::any result);
            void SetErrorMsg(std::string errMsg);
            void SetSuccess(bool success);
        };

        class HttpResult:virtual public NetResult
        {
        public:
            using ptr = std::shared_ptr<HttpResult>;
            using wptr = std::weak_ptr<HttpResult>;
            HttpResult();
            //需要对象接收
            protocol::http::HttpResponse GetResponse();
        };

        class HttpResultInner: public HttpResult, public NetResultInner
        {
        public:
            using ptr = std::shared_ptr<HttpResultInner>;
            using wptr = std::weak_ptr<HttpResultInner>;
        };

        class SmtpResult:virtual public NetResult
        {
        public:
            using ptr = std::shared_ptr<SmtpResult>;
            using wptr = std::weak_ptr<SmtpResult>;
            SmtpResult();
            std::queue<protocol::smtp::SmtpResponse> GetResponse();
        };

        class SmtpResultInner: public SmtpResult, public NetResultInner
        {
        public:
            using ptr = std::shared_ptr<SmtpResultInner>;
            using wptr = std::weak_ptr<SmtpResultInner>;
        };

        //udp
        struct UdpResInfo
        {
            std::string m_buffer;
            std::string m_host;
            uint16_t m_port;
        };

        class DnsResult:virtual public NetResult
        {
        public:
            using ptr = std::shared_ptr<DnsResult>;
            using wptr = std::weak_ptr<DnsResult>;
            DnsResult();
            bool HasCName();
            std::string GetCName();
            bool HasA();
            std::string GetA();
            bool HasAAAA();
            std::string GetAAAA();
            bool HasPtr();
            std::string GetPtr();
        private:
            void Parse();
        private:
            bool m_isParsed;
            std::queue<std::string> m_cNames;
            std::queue<std::string> m_a;
            std::queue<std::string> m_aaaa;
            std::queue<std::string> m_ptr;
        };

        class DnsResultInner: public DnsResult, public NetResultInner
        {
        public:
            using ptr = std::shared_ptr<DnsResultInner>;
            using wptr = std::weak_ptr<DnsResultInner>;
        };
    }
}


#endif