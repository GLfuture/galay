#ifndef GALAY_CLIENT_H
#define GALAY_CLIENT_H
#include "scheduler.h"

namespace galay
{
    class GY_TcpClientBase{
    protected:
        virtual int Connect() = 0;
    };

    class GY_UdpClientBase{
    
    };

    class GY_HttpAsyncClient: public GY_TcpClientBase{
    public:
        using ptr = std::shared_ptr<GY_HttpAsyncClient>;
        using wptr = std::weak_ptr<GY_HttpAsyncClient>;
        using uptr = std::unique_ptr<GY_HttpAsyncClient>;
        GY_HttpAsyncClient(std::string url);
        void KeepAlive();
        void CancleKeepalive();
        
        //void Upgrade(const std::string& version);
        //void Downgrade(const std::string& version);
        //To Do
        void Chunked(const std::string& chunked_data);

        //return http_response ptr
        HttpAwaiter Get(protocol::http::Http1_1_Request::ptr request = nullptr);
        HttpAwaiter Post(std::string&& body,protocol::http::Http1_1_Request::ptr request = nullptr);
        HttpAwaiter Options(protocol::http::Http1_1_Request::ptr request = nullptr);
        void Close();
        virtual ~GY_HttpAsyncClient() = default;
    private:
        virtual int Connect() override;
        protocol::http::Http1_1_Response::ptr ExecMethod(std::string reqStr);
    private:
        int m_fd;
        std::string m_host;
        uint16_t m_port;
        std::string m_uri;
        std::string m_version;
        std::function<protocol::http::Http1_1_Response::ptr()> m_ExecMethod;
        bool m_keepalive;
        bool m_isconnected;
    };

    class GY_SmtpAsyncClient : public GY_TcpClientBase {
    public:
        GY_SmtpAsyncClient(const std::string& host, uint16_t port);
        SmtpAwaiter Auth(std::string account,std::string password);
        SmtpAwaiter SendEmail(std::string FromEmail,const std::vector<std::string>& ToEmails,galay::protocol::smtp::SmtpMsgInfo msg);
        SmtpAwaiter Quit();
        void Close();
        virtual ~GY_SmtpAsyncClient() = default;
    private:
        virtual int Connect() override;
        std::vector<galay::protocol::smtp::Smtp_Response::ptr> ExecSendMsg(std::queue<std::string> requests);
    private:
        int m_fd;
        std::string m_host;
        uint16_t m_port;
        bool m_isconnected;
        std::function<std::vector<protocol::smtp::Smtp_Response::ptr>()> m_ExecSendMsg;
    };

    class DnsAsyncClient : public galay::GY_UdpClientBase {
    public:
        DnsAsyncClient(const std::string& host, uint16_t port = 53);
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
        std::function<galay::protocol::dns::Dns_Response::ptr()> m_ExecSendMsg; 
    };
}

#endif