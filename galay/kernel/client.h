#ifndef GALAY_CLIENT_H
#define GALAY_CLIENT_H
#include "../common/threadpool.h"
#include "../common/awaiter.h"
#include <openssl/ssl.h>

namespace galay
{
    namespace common{
        class WaitGroup;
    }

    namespace kernel
    {   
        class GY_IOScheduler;
        class GY_RecvTask;
        class GY_SendTask;
        class TcpResult;
        class HttpResult;
        class SmtpResult;

        //noblock
        class GY_TcpClient
        {
        public:
            using ptr = std::shared_ptr<GY_TcpClient>;
            using wptr = std::weak_ptr<GY_TcpClient>;
            using uptr = std::unique_ptr<GY_TcpClient>;
            GY_TcpClient(std::weak_ptr<GY_IOScheduler> scheduler);
            void IsSSL(bool isSSL);
            common::GY_NetCoroutine Unary(std::string host, uint16_t port, std::string &&buffer, std::shared_ptr<TcpResult> result, bool autoClose);
            common::GY_NetCoroutine Unary(std::string host, uint16_t port, std::queue<std::string> requests, std::shared_ptr<TcpResult> result, bool autoClose);
            void CloseConn();
            virtual ~GY_TcpClient();
        private:
            //simple
            std::shared_ptr<TcpResult> Socket();
            std::shared_ptr<TcpResult> Connect(const std::string &ip, uint16_t port);
            std::shared_ptr<TcpResult> Send(std::string &&buffer);
            std::shared_ptr<TcpResult> Recv(std::string &buffer);
            std::shared_ptr<TcpResult> Close();
            //ssl
            std::shared_ptr<TcpResult> SSLSocket(long minVersion, long maxVersion);
            std::shared_ptr<TcpResult> SSLConnect(const std::string &ip, uint16_t port);
            std::shared_ptr<TcpResult> SSLSend(std::string &&buffer);
            std::shared_ptr<TcpResult> SSLRecv(std::string &buffer);
            std::shared_ptr<TcpResult> SSLClose();

            bool Socketed();
            bool Connected();
        private:
            void SetResult(std::shared_ptr<TcpResult> result, bool success, std::string &&errMsg);
        private:
            int m_fd;
            bool m_isSSL;
            bool m_isSocket;
            bool m_isConnected;
            SSL *m_ssl = nullptr;
            SSL_CTX *m_ctx = nullptr;
            std::weak_ptr<GY_IOScheduler> m_scheduler;
            common::GY_ThreadPool::ptr m_threadPool;
            std::shared_ptr<GY_RecvTask> m_recvTask;
            std::shared_ptr<GY_SendTask> m_sendTask;
        };

        class GY_HttpAsyncClient
        {
        public:
            using ptr = std::shared_ptr<GY_HttpAsyncClient>;
            using wptr = std::weak_ptr<GY_HttpAsyncClient>;
            using uptr = std::unique_ptr<GY_HttpAsyncClient>;
            GY_HttpAsyncClient(std::weak_ptr<GY_IOScheduler> scheduler);
            // return response ptr
            std::shared_ptr<HttpResult> Get(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose = false);
            std::shared_ptr<HttpResult> Post(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose = false);
            std::shared_ptr<HttpResult> Options(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose = false);
            std::shared_ptr<HttpResult> Put(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose = false);
            std::shared_ptr<HttpResult> Delete(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose = false);
            std::shared_ptr<HttpResult> Head(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose = false);
            std::shared_ptr<HttpResult> Trace(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose = false);
            std::shared_ptr<HttpResult> Patch(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose = false);
            void CloseConn();
            virtual ~GY_HttpAsyncClient() = default;
        private:
            std::tuple<std::string,uint16_t,std::string> ParseUrl(const std::string& url);
        private:
            GY_TcpClient::ptr m_tcpClient;
        };

        class GY_SmtpAsyncClient
        {
        public:
            using ptr = std::shared_ptr<GY_SmtpAsyncClient>;
            using wptr = std::weak_ptr<GY_SmtpAsyncClient>;
            using uptr = std::unique_ptr<GY_SmtpAsyncClient>;
            GY_SmtpAsyncClient(std::weak_ptr<GY_IOScheduler> scheduler);

            void Connect(const std::string& url);
            std::shared_ptr<SmtpResult> Auth(std::string account, std::string password);
            std::shared_ptr<SmtpResult> SendEmail(std::string FromEmail, const std::vector<std::string> &ToEmails, galay::protocol::smtp::SmtpMsgInfo msg);
            std::shared_ptr<SmtpResult> Quit();
            void CloseConn();
            virtual ~GY_SmtpAsyncClient() = default;
        private:
            std::tuple<std::string,uint16_t> ParseUrl(const std::string& url);
        private:
            std::string m_host;
            uint16_t m_port;
            GY_TcpClient::ptr m_tcpClient;
        };

        class DnsAsyncClient
        {
        public:
            DnsAsyncClient(const std::string &host, uint16_t port = 53);
            common::DnsAwaiter QueryA(std::queue<std::string> domains);
            common::DnsAwaiter QueryAAAA(std::queue<std::string> domains);
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