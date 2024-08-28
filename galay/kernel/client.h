#ifndef GALAY_CLIENT_H
#define GALAY_CLIENT_H
#include "../common/awaiter.h"
#include "../common/result.h"
#include <openssl/ssl.h>

namespace galay::coroutine
{
    class WaitGroup;
}

namespace galay::poller
{
    class IOScheduler;
}

namespace galay::client
{
    class NetResult
    {
    public:
        NetResult(result::ResultInterface::ptr result);
        bool Success();
        std::string Error();
        coroutine::GroupAwaiter& Wait();
    private:
        result::ResultInterface::ptr m_result;
    };

    //noblock
    class TcpClient
    {
    public:
        using ptr = std::shared_ptr<TcpClient>;
        using wptr = std::weak_ptr<TcpClient>;
        using uptr = std::unique_ptr<TcpClient>;
        TcpClient(std::weak_ptr<poller::IOScheduler> scheduler);
        void IsSSL(bool isSSL);
        coroutine::NetCoroutine Unary(std::string host, uint16_t port, std::string &&buffer, std::shared_ptr<result::ResultInterface> result, bool autoClose);
        coroutine::NetCoroutine Unary(std::string host, uint16_t port, std::queue<std::string> requests, std::shared_ptr<result::ResultInterface> result, bool autoClose);
        NetResult CloseConn();
        virtual ~TcpClient();
    private:
        //simple
        NetResult Socket();
        NetResult Connect(const std::string &ip, uint16_t port);
        NetResult Send(std::string &&buffer);
        NetResult Recv(std::string &buffer);
        NetResult Close();
        //ssl
        NetResult SSLSocket(long minVersion, long maxVersion);
        NetResult SSLConnect(const std::string &ip, uint16_t port);
        NetResult SSLSend(std::string &&buffer);
        NetResult SSLRecv(std::string &buffer);
        NetResult SSLClose();

        bool Socketed();
        bool Connected();
    private:
        void SetResult(std::shared_ptr<result::ResultInterface> result, bool success, std::string &&errMsg);
    private:
        int m_fd;
        bool m_isSSL;
        bool m_isSocket;
        bool m_isConnected;
        SSL *m_ssl = nullptr;
        std::string m_wbuffer;
        std::string m_rbuffer;
        SSL_CTX *m_ctx = nullptr;
        std::weak_ptr<poller::IOScheduler> m_scheduler;
    };

    class HttpResult
    {
    public:
        HttpResult(result::ResultInterface::ptr result);
        bool Success();
        std::string Error();
        coroutine::GroupAwaiter& Wait();
        protocol::http::HttpResponse GetResponse();
    private:
        result::ResultInterface::ptr m_result;
    };

    class HttpAsyncClient
    {
    public:
        using ptr = std::shared_ptr<HttpAsyncClient>;
        using wptr = std::weak_ptr<HttpAsyncClient>;
        using uptr = std::unique_ptr<HttpAsyncClient>;
        HttpAsyncClient(std::weak_ptr<poller::IOScheduler> scheduler);
        // return response ptr
        HttpResult Get(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose = false);
        HttpResult Post(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose = false);
        HttpResult Options(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose = false);
        HttpResult Put(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose = false);
        HttpResult Delete(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose = false);
        HttpResult Head(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose = false);
        HttpResult Trace(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose = false);
        HttpResult Patch(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose = false);
        NetResult CloseConn();
        virtual ~HttpAsyncClient() = default;
    private:
        std::tuple<std::string,uint16_t,std::string> ParseUrl(const std::string& url);
    private:
        TcpClient::ptr m_tcpClient;
    };

    class SmtpResult
    {
    public:
        SmtpResult(result::ResultInterface::ptr result);
        bool Success();
        std::string Error();
        coroutine::GroupAwaiter& Wait();
        std::queue<protocol::smtp::SmtpResponse> GetResponse();
    private:
        result::ResultInterface::ptr m_result;
    };

    class SmtpAsyncClient
    {
    public:
        using ptr = std::shared_ptr<SmtpAsyncClient>;
        using wptr = std::weak_ptr<SmtpAsyncClient>;
        using uptr = std::unique_ptr<SmtpAsyncClient>;
        SmtpAsyncClient(std::weak_ptr<poller::IOScheduler> scheduler);

        void Connect(const std::string& url);
        SmtpResult Auth(std::string account, std::string password);
        SmtpResult SendEmail(std::string FromEmail, const std::vector<std::string> &ToEmails, galay::protocol::smtp::SmtpMsgInfo msg);
        SmtpResult Quit();
        NetResult CloseConn();
        virtual ~SmtpAsyncClient() = default;
    private:
        std::tuple<std::string,uint16_t> ParseUrl(const std::string& url);
    private:
        std::string m_host;
        uint16_t m_port;
        TcpClient::ptr m_tcpClient;
    };

    struct UdpRespAddrInfo
    {
        std::string m_buffer;
        std::string m_host;
        uint16_t m_port;
    };

    class UdpClient
    {
    public:
        using ptr = std::shared_ptr<UdpClient>;
        using wptr = std::weak_ptr<UdpClient>;
        UdpClient(std::weak_ptr<poller::IOScheduler> scheduler);
        coroutine::NetCoroutine Unary(std::string host, uint16_t port, std::string &&buffer, std::shared_ptr<result::ResultInterface> result);
        NetResult CloseSocket();
        bool Socketed();
        virtual ~UdpClient();
    private:
        NetResult Socket();
        NetResult SendTo(std::string host, uint16_t port,std::string&& buffer);
        NetResult RecvFrom(std::string& host, uint16_t& port, std::string& buffer);
        void SetResult(std::shared_ptr<result::ResultInterface> result, bool success, std::string &&errMsg);
    private:
        int m_fd;
        bool m_isSocketed;
        std::string m_request;
        std::weak_ptr<poller::IOScheduler> m_scheduler;
    };

    class DnsResult
    {
    public:
        DnsResult(result::ResultInterface::ptr result);
        bool Success();
        std::string Error();
        coroutine::GroupAwaiter& Wait();
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
        result::ResultInterface::ptr m_result;
    };

    class DnsAsyncClient
    {
    public:
        using ptr = std::shared_ptr<DnsAsyncClient>;
        using wptr = std::weak_ptr<DnsAsyncClient>;
        using uptr = std::unique_ptr<DnsAsyncClient>;
        DnsAsyncClient(std::weak_ptr<poller::IOScheduler> scheduler);
        DnsResult QueryA(const std::string& host, const uint16_t& port, const std::string& domain);
        DnsResult QueryAAAA(const std::string& host, const uint16_t& port, const std::string& domain);
        DnsResult QueryPtr(const std::string& host, const uint16_t& port, const std::string& want);
        NetResult CloseSocket();
        virtual ~DnsAsyncClient() = default;
    private:
        std::string HostToPtr(const std::string& host);
    private:
        UdpClient::ptr m_udpClient;
    };
}

#endif