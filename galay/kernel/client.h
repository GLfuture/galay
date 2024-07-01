#ifndef GALAY_CLIENT_H
#define GALAY_CLIENT_H
#include "scheduler.h"

namespace galay
{
    namespace kernel
    {
        class GY_ClientBase
        {
        public:
            virtual ~GY_ClientBase() = default;
        };

        class GY_HttpAsyncClient : public GY_ClientBase
        {
        public:
            using ptr = std::shared_ptr<GY_HttpAsyncClient>;
            using wptr = std::weak_ptr<GY_HttpAsyncClient>;
            using uptr = std::unique_ptr<GY_HttpAsyncClient>;
            GY_HttpAsyncClient();
            void KeepAlive();
            void CancleKeepalive();

            // void Upgrade(const std::string& version);
            // void Downgrade(const std::string& version);
            // To Do
            void Chunked(const std::string &chunked_data);
            void SetVersion(const std::string &version);
            void AddHeaderPair(const std::string &key, const std::string &value);
            void RemoveHeaderPair(const std::string &key);
            // return http_response ptr
            HttpAwaiter Get(const std::string &url);
            HttpAwaiter Post(const std::string &url, std::string &&body);
            HttpAwaiter Options(const std::string &url);
            void Close();
            virtual ~GY_HttpAsyncClient() = default;

        private:
            virtual int Connect(const std::string &ip, uint16_t port);
            protocol::http::Http1_1_Response::ptr ExecMethod(std::string reqStr);
            // addr  port  uri
            std::tuple<std::string, uint16_t, std::string> ParseUrl(const std::string &url);
            void SetHttpHeaders(protocol::http::Http1_1_Request::ptr request);

        private:
            int m_fd;
            std::string m_version;
            std::queue<std::future<void>> m_futures;
            std::function<protocol::http::Http1_1_Response::ptr()> m_ExecMethod;
            std::unordered_map<std::string, std::string> m_headers;
            bool m_keepalive;
            bool m_isconnected;
        };

        class GY_SmtpAsyncClient : public GY_ClientBase
        {
        public:
            GY_SmtpAsyncClient(const std::string &host, uint16_t port);
            SmtpAwaiter Auth(std::string account, std::string password);
            SmtpAwaiter SendEmail(std::string FromEmail, const std::vector<std::string> &ToEmails, galay::protocol::smtp::SmtpMsgInfo msg);
            SmtpAwaiter Quit();
            void Close();
            virtual ~GY_SmtpAsyncClient() = default;

        private:
            virtual int Connect();
            std::vector<galay::protocol::smtp::Smtp_Response::ptr> ExecSendMsg(std::queue<std::string> requests);

        private:
            int m_fd;
            std::string m_host;
            uint16_t m_port;
            bool m_isconnected;
            std::queue<std::future<void>> m_futures;
            std::function<std::vector<protocol::smtp::Smtp_Response::ptr>()> m_ExecSendMsg;
        };

        class DnsAsyncClient : public GY_ClientBase
        {
        public:
            DnsAsyncClient(const std::string &host, uint16_t port = 53);
            DnsAwaiter QueryA(std::queue<std::string> domains);
            DnsAwaiter QueryAAAA(std::queue<std::string> domains);
            void Close();
            virtual ~DnsAsyncClient() = default;

        private:
            galay::protocol::dns::Dns_Response::ptr ExecSendMsg(std::queue<std::string> reqStr);

        private:
            int m_fd;
            bool m_IsInit;
            uint16_t m_port;
            std::string m_host;
            std::queue<std::future<void>> m_futures;
            std::function<galay::protocol::dns::Dns_Response::ptr()> m_ExecSendMsg;
        };
    }
}

#endif