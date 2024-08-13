#ifndef GALAY_CLIENT_H
#define GALAY_CLIENT_H
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
        class GY_TcpRecvTask;
        class GY_TcpSendTask;
        class NetResult;
        class HttpResult;
        class SmtpResult;
        class DnsResult;

        //noblock
        class GY_TcpClient
        {
        public:
            using ptr = std::shared_ptr<GY_TcpClient>;
            using wptr = std::weak_ptr<GY_TcpClient>;
            using uptr = std::unique_ptr<GY_TcpClient>;
            GY_TcpClient(std::weak_ptr<GY_IOScheduler> scheduler);
            void IsSSL(bool isSSL);
            common::GY_NetCoroutine Unary(std::string host, uint16_t port, std::string &&buffer, std::shared_ptr<NetResult> result, bool autoClose);
            common::GY_NetCoroutine Unary(std::string host, uint16_t port, std::queue<std::string> requests, std::shared_ptr<NetResult> result, bool autoClose);
            std::shared_ptr<NetResult> CloseConn();
            virtual ~GY_TcpClient();
        private:
            //simple
            std::shared_ptr<NetResult> Socket();
            std::shared_ptr<NetResult> Connect(const std::string &ip, uint16_t port);
            std::shared_ptr<NetResult> Send(std::string &&buffer);
            std::shared_ptr<NetResult> Recv(std::string &buffer);
            std::shared_ptr<NetResult> Close();
            //ssl
            std::shared_ptr<NetResult> SSLSocket(long minVersion, long maxVersion);
            std::shared_ptr<NetResult> SSLConnect(const std::string &ip, uint16_t port);
            std::shared_ptr<NetResult> SSLSend(std::string &&buffer);
            std::shared_ptr<NetResult> SSLRecv(std::string &buffer);
            std::shared_ptr<NetResult> SSLClose();

            bool Socketed();
            bool Connected();
        private:
            void SetResult(std::shared_ptr<NetResult> result, bool success, std::string &&errMsg);
        private:
            int m_fd;
            bool m_isSSL;
            bool m_isSocket;
            bool m_isConnected;
            SSL *m_ssl = nullptr;
            SSL_CTX *m_ctx = nullptr;
            std::weak_ptr<GY_IOScheduler> m_scheduler;
            std::shared_ptr<GY_TcpRecvTask> m_recvTask;
            std::shared_ptr<GY_TcpSendTask> m_sendTask;
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
            std::shared_ptr<NetResult>  CloseConn();
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
            std::shared_ptr<NetResult> CloseConn();
            virtual ~GY_SmtpAsyncClient() = default;
        private:
            std::tuple<std::string,uint16_t> ParseUrl(const std::string& url);
        private:
            std::string m_host;
            uint16_t m_port;
            GY_TcpClient::ptr m_tcpClient;
        };

        class GY_UdpClient
        {
        public:
            using ptr = std::shared_ptr<GY_UdpClient>;
            using wptr = std::weak_ptr<GY_UdpClient>;
            GY_UdpClient(std::weak_ptr<GY_IOScheduler> scheduler);
            common::GY_NetCoroutine Unary(std::string host, uint16_t port, std::string &&buffer, std::shared_ptr<NetResult> result);
            std::shared_ptr<NetResult> CloseSocket();
            bool Socketed();
            virtual ~GY_UdpClient();
        private:
            std::shared_ptr<NetResult> Socket();
            std::shared_ptr<NetResult> SendTo(std::string host, uint16_t port,std::string&& buffer);
            std::shared_ptr<NetResult> RecvFrom(std::string& host, uint16_t& port, std::string& buffer);
            void SetResult(std::shared_ptr<NetResult> result, bool success, std::string &&errMsg);
        private:
            int m_fd;
            bool m_isSocketed;
            std::string m_request;
            std::weak_ptr<GY_IOScheduler> m_scheduler;
        };

        class DnsAsyncClient
        {
        public:
            using ptr = std::shared_ptr<DnsAsyncClient>;
            using wptr = std::weak_ptr<DnsAsyncClient>;
            using uptr = std::unique_ptr<DnsAsyncClient>;
            DnsAsyncClient(std::weak_ptr<GY_IOScheduler> scheduler);
            std::shared_ptr<DnsResult> QueryA(const std::string& host, const uint16_t& port, const std::string& domain);
            std::shared_ptr<DnsResult> QueryAAAA(const std::string& host, const uint16_t& port, const std::string& domain);
            std::shared_ptr<DnsResult> QueryPtr(const std::string& host, const uint16_t& port, const std::string& want);
            std::shared_ptr<NetResult> CloseSocket();
            virtual ~DnsAsyncClient() = default;
        private:
            std::string HostToPtr(const std::string& host);
        private:
            GY_UdpClient::ptr m_udpClient;
        };
    }
}

#endif