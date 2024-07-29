#ifndef GALAY_CLIENT_H
#define GALAY_CLIENT_H
#include "scheduler.h"
#include "../common/threadpool.h"

namespace galay
{
    namespace common{
        class WaitGroup;
    }

    namespace kernel
    {   
        class GY_RecvTask;
        class GY_SendTask;
        class GY_TcpClient;
        class GY_HttpAsyncClient;

        class TcpResult
        {
            friend class GY_TcpClient;
            friend class GY_HttpAsyncClient;
        public:
            using ptr = std::shared_ptr<TcpResult>;
            using wptr = std::weak_ptr<TcpResult>;
            TcpResult(common::GY_NetCoroutinePool::wptr coPool);
            bool Success();
            std::string Error();
            void AddTaskNum(uint16_t taskNum);
            galay::common::GroupAwaiter& Wait();
            void Done();

        protected:
            bool m_success;
            std::string m_errMsg;
            std::shared_ptr<common::WaitGroup> m_waitGroup;
        };
        //noblock
        class GY_TcpClient
        {
        public:
            using ptr = std::shared_ptr<GY_TcpClient>;
            using wptr = std::weak_ptr<GY_TcpClient>;
            using uptr = std::unique_ptr<GY_TcpClient>;
            GY_TcpClient(common::GY_NetCoroutinePool::wptr coPool, GY_IOScheduler::wptr scheduler);
            //simple
            TcpResult::ptr Socket();
            TcpResult::ptr Connect(const std::string &ip, uint16_t port);
            TcpResult::ptr Send(std::string &&buffer);
            TcpResult::ptr Recv(std::string &buffer);
            TcpResult::ptr Close();
            //ssl
            TcpResult::ptr SSLSocket(long minVersion, long maxVersion);
            TcpResult::ptr SSLConnect(const std::string &ip, uint16_t port);
            TcpResult::ptr SSLSend(std::string &&buffer);
            TcpResult::ptr SSLRecv(std::string &buffer);
            TcpResult::ptr SSLClose();

            bool Connected();
            common::GY_NetCoroutinePool::wptr GetCoPool();

            virtual ~GY_TcpClient();
        private:
            void SetResult(TcpResult::ptr result, bool success, std::string &&errMsg);
        private:
            int m_fd;
            bool m_isConnected;
            SSL_CTX *m_ctx = nullptr;
            SSL *m_ssl = nullptr;
            GY_IOScheduler::wptr m_scheduler;
            common::GY_NetCoroutinePool::wptr m_coPool;
            common::GY_ThreadPool::ptr m_threadPool;
            std::shared_ptr<GY_RecvTask> m_recvTask;
            std::shared_ptr<GY_SendTask> m_sendTask;
        };
        
        class HttpResult: public TcpResult
        {
            friend class GY_HttpAsyncClient;
        public:
            using ptr = std::shared_ptr<HttpResult>;
            using wptr = std::weak_ptr<HttpResult>;
            HttpResult(common::GY_NetCoroutinePool::wptr coPool);
            protocol::http::HttpResponse::ptr GetResponse();
        private:
            protocol::http::HttpResponse::ptr m_response;
        };

        class GY_HttpAsyncClient
        {
        public:
            using ptr = std::shared_ptr<GY_HttpAsyncClient>;
            using wptr = std::weak_ptr<GY_HttpAsyncClient>;
            using uptr = std::unique_ptr<GY_HttpAsyncClient>;
            GY_HttpAsyncClient(common::GY_NetCoroutinePool::wptr coPool, GY_IOScheduler::wptr scheduler);
            // return response ptr
            HttpResult::ptr Get(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose = false);
            HttpResult::ptr Post(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose = false);
            HttpResult::ptr Options(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose = false);
            HttpResult::ptr Put(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose = false);
            HttpResult::ptr Delete(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose = false);
            HttpResult::ptr Head(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose = false);
            HttpResult::ptr Trace(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose = false);
            HttpResult::ptr Patch(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose = false);
            virtual ~GY_HttpAsyncClient() = default;
        private:
            
            common::GY_NetCoroutine<common::CoroutineStatus> Unary(std::string host, uint16_t port, std::string &&request, HttpResult::ptr result, bool autoClose);
            std::tuple<std::string,uint16_t,std::string> ParseUrl(const std::string& url);
        private:
            GY_TcpClient::ptr m_tcpClient;
        };

        class GY_SmtpAsyncClient
        {
        public:
            GY_SmtpAsyncClient(const std::string &host, uint16_t port);
            common::SmtpAwaiter Auth(std::string account, std::string password);
            common::SmtpAwaiter SendEmail(std::string FromEmail, const std::vector<std::string> &ToEmails, galay::protocol::smtp::SmtpMsgInfo msg);
            common::SmtpAwaiter Quit();
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