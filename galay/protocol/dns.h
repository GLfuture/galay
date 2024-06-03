#ifndef GALAY_DNS_H
#define GALAY_DNS_H
#include "basic_protocol.h"
#include "../util/stringutil.h"
#include <queue>
#include <string.h>
#include <arpa/inet.h>

namespace galay
{
    namespace protocol
    {
        namespace dns
        {
            #pragma pack(push, 1)
            #define MAX_UDP_LENGTH 1500
            enum DNS_Query_Type
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

            struct Dns_Flags
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

            struct Dns_Header
            {
                unsigned short m_id;                 // 会话ID
                Dns_Flags m_flags{0};                // flags
                unsigned short m_questions = 0;      // 问题数
                unsigned short m_answers_RRs = 0;    // 回答资源记录数
                unsigned short m_authority_RRs = 0;  // 授权资源记录数
                unsigned short m_additional_RRs = 0; // 附加资源记录数
            };

            struct Dns_Question
            {
                ::std::string m_qname;
                unsigned short m_type = 0;
                unsigned short m_class = htons(1); // 一般为1
            };

            struct Dns_Answer
            {
                ::std::string m_aname;
                unsigned short m_type = 0;
                unsigned short m_class = htons(1);
                unsigned int m_ttl = 0;
                unsigned short m_data_len = 0;
                ::std::string m_data;
            };

            #pragma pack()

            class Dns_Protocol
            {
            public:
                Dns_Header GetHeader();
                void SetHeader(Dns_Header &&header);
                void SetQuestionQueue(::std::queue<Dns_Question> &&questions);
                ::std::queue<Dns_Answer> GetAnswerQueue();

            protected:
                Dns_Header m_header{0};
                ::std::queue<Dns_Question> m_questions;
                ::std::queue<Dns_Answer> m_answers;
            };

            class Dns_Request : public Dns_Protocol, public GY_UdpRequest, public GY_UdpResponse
            {
            public:
                using ptr = ::std::shared_ptr<Dns_Request>;
                // ignore
                int DecodePdu(::std::string &buffer) override { return 0; }
                ::std::string EncodePdu() override;
                virtual void Clear() override;

            protected:
                ::std::string ModifyHostname(::std::string hostname);
            };

            class Dns_Response : public Dns_Protocol, public GY_UdpRequest, public GY_UdpResponse
            {
            public:
                using ptr = ::std::shared_ptr<Dns_Response>;
                // ignore
                ::std::string EncodePdu() override { return ""; }
                int DecodePdu(::std::string &buffer) override;
                virtual void Clear() override;

            protected:
                static bool IsPointer(int in);
                ::std::string DealAnswer(unsigned short type, char *buffer, ::std::string data);
                int DnsParseName(unsigned char *buffer, unsigned char *ptr, ::std::string &out);
            };
        }
    }
}

#endif