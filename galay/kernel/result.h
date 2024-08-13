#ifndef GALAY_RESULT_H
#define GALAY_RESULT_H

#include <memory>
#include <any>
#include "../protocol/http.h"
#include "../protocol/smtp.h"
#include "../protocol/dns.h"

namespace galay{
    namespace common{
        class GroupAwaiter;
        class WaitGroup;
    }

    namespace kernel{
        class GY_HttpAsyncClient; 
        class GY_SmtpAsyncClient;
        class GY_TcpClient;
        class GY_UdpClient;
        class GY_TcpConnector;

        class NetResult
        {
            friend class kernel::GY_TcpClient;
            friend class kernel::GY_HttpAsyncClient;
            friend class kernel::GY_TcpConnector;
            friend class kernel::GY_UdpClient;
        public:
            using ptr = std::shared_ptr<NetResult>;
            using wptr = std::weak_ptr<NetResult>;
            NetResult();
            void Done();
            bool Success();
            std::string Error();
            void AddTaskNum(uint16_t taskNum);
            common::GroupAwaiter& Wait();
        protected:
            bool m_success;
            //tcp:一次请求(std::string) ,多次请求(std::queue<std::string>)
            //udp:struct UdpResInfo
            std::any m_result;   
            std::string m_errMsg;
            std::shared_ptr<common::WaitGroup> m_waitGroup;
        };

        class HttpResult: public NetResult
        {
            friend class kernel::GY_HttpAsyncClient;
        public:
            using ptr = std::shared_ptr<HttpResult>;
            using wptr = std::weak_ptr<HttpResult>;
            HttpResult();
            //需要对象接收
            protocol::http::HttpResponse GetResponse();
        };

        class SmtpResult: public NetResult
        {
            friend class kernel::GY_SmtpAsyncClient;
        public:
            using ptr = std::shared_ptr<SmtpResult>;
            using wptr = std::weak_ptr<SmtpResult>;
            SmtpResult();
            std::queue<protocol::smtp::SmtpResponse> GetResponse();
        };


        //udp
        struct UdpResInfo
        {
            std::string m_buffer;
            std::string m_host;
            uint16_t m_port;
        };

        class DnsResult: public NetResult
        {
            friend class kernel::GY_SmtpAsyncClient;
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

    }
}


#endif