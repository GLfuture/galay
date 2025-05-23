#ifndef __GALAY_DNS_H__
#define __GALAY_DNS_H__
#include <memory>
#include <queue>
#include <string>
#include <string.h>
#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__)
    #include <arpa/inet.h>
#elif  defined(WIN32) || defined(_WIN32) || defined(_WIN32_) || defined(WIN64) || defined(_WIN64) || defined(_WIN64_)
    #include <WinSock2.h>
    #include <ws2tcpip.h>
#endif // 

namespace galay::error
{
    enum DnsErrorCode {
        kDnsError_NoError = 0,
    };

    class DnsError
    {
    public:
        using ptr = std::shared_ptr<DnsError>;
        using wptr = std::weak_ptr<DnsError>;
        using uptr = std::unique_ptr<DnsError>;
        bool HasError() const;
        DnsErrorCode& Code();
        void Reset();
        std::string ToString(DnsErrorCode code) const;
    protected:
        DnsErrorCode m_code = kDnsError_NoError;
    };
}

namespace galay::dns
{
    #pragma pack(push, 1)
    #define MAX_UDP_LENGTH 1500
    enum DnsQueryType
    {
        kDnsQueryA = 1,      // 由域名获取ipv4地址
        kDnsQueryNS = 2,     // 查询域名服务器
        kDnsQueryCname = 5,  // 查询规范名称
        kDnsQuerySOA = 6,    // 开始授权
        kDnsQueryWKS = 11,   // 熟知服务
        kDnsQueryPtr = 12,   // ip地址转域名
        kDnsQueryHInfo = 13, // 主机信息
        kDnsQueryMX = 15,    // 邮件交换
        kDnsQueryAAAA = 28,  // 由域名获取ipv6地址
        kDnsQueryAXFR = 252, // 传送整个区的请求
        kDnsQueryAny = 255,  // 对所有记录的请求
    };

    struct DnsFlags
    {
        unsigned char m_qr : 1; // 0为查询，1为响应
        unsigned char m_opcode : 4;
        unsigned char m_aa : 1;    // 授权问答
        unsigned char m_tc : 1;    // 可截断的
        unsigned char m_rd : 1;    // 期望递归
        unsigned char m_ra : 1;    // 可用递归
        unsigned char m_zero : 3;  // 保留字段为0
        unsigned char m_rcode : 4; // 返回码,0无错,3名字错,2服务器错
    };

    struct DnsHeader
    {
        void Reset();
        unsigned short m_id;                    // 会话ID
        DnsFlags m_flags{0};                    // flags
        unsigned short m_questions = 0;         // 问题数
        unsigned short m_answers_RRs = 0;       // 回答资源记录数
        unsigned short m_authority_RRs = 0;     // 授权资源记录数
        unsigned short m_additional_RRs = 0;    // 附加资源记录数
    };

    struct DnsQuestion
    {
        std::string m_qname;
        unsigned short m_type = 0;
        unsigned short m_class = htons(1); // 一般为1
    };

    struct DnsAnswer
    {
        std::string m_aname;
        unsigned short m_type = 0;
        unsigned short m_class = htons(1);
        unsigned int m_ttl = 0;
        unsigned short m_data_len = 0;
        std::string m_data;
    };

    #pragma pack(pop)

    extern std::string GetDnsAnswerTypeString(unsigned short type);

    class DnsProtocol
    {
    public:
        DnsProtocol();
        DnsHeader GetHeader();
        void SetHeader(DnsHeader &&header);
        void SetQuestion(DnsQuestion &&question);
        std::queue<DnsAnswer> GetAnswerQueue();

    protected:
        DnsHeader m_header{0};
        error::DnsError::ptr m_error;
        DnsQuestion m_question;
        std::queue<DnsAnswer> m_answers;
    };

    class DnsRequest: public DnsProtocol
    {
    public:
        using ptr = std::shared_ptr<DnsRequest>;
        [[nodiscard]] std::string EncodePdu() const;
        bool DecodePdu(const std::string_view &buffer);
        [[nodiscard]] bool HasError() const;
        [[nodiscard]] int GetErrorCode() const;
        std::string GetErrorString();
        void Reset();
    protected:
        std::string ModifyHostname(std::string hostname) const;
        int DnsParseName(unsigned char *buffer, unsigned char *ptr, std::string &out);
        bool IsPointer(int in);
    };

    class DnsResponse: public DnsProtocol
    {
    public:
        using ptr = std::shared_ptr<DnsResponse>;
        bool DecodePdu(const std::string_view &buffer);
        // ignore
        [[nodiscard]] std::string EncodePdu() const;
        [[nodiscard]] bool HasError() const;
        [[nodiscard]] int GetErrorCode() const;
        std::string GetErrorString();
        void Reset();
    protected:
        std::string ModifyHostname(std::string hostname);
        static bool IsPointer(int in);
        std::string DealAnswer(unsigned short type, char *buffer, std::string data);
        int DnsParseName(unsigned char *buffer, unsigned char *ptr, std::string &out);
    };
}


#endif