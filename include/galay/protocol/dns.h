#ifndef GALAY_DNS_H
#define GALAY_DNS_H
#include "basic_protocol.h"
#include "../config/config.h"
#include "../util/stringutil.h"
#include <queue>
#include <string.h>
#include <arpa/inet.h>


namespace galay
{
    namespace protocol{
    #pragma pack(push,1)

        enum DNS_Query_Type{
            DNS_QUERY_A=1,          //由域名获取ipv4地址
            DNS_QUERY_NS=2,         //查询域名服务器
            DNS_QUERY_CNAME=5,      //查询规范名称
            DNS_QUERY_SOA=6,        //开始授权
            DNS_QUERY_WKS=11,       //熟知服务
            DNS_QUERY_PTR=12,       //ip地址转域名
            DNS_QUERY_HINFO=13,     //主机信息
            DNS_QUERY_MX=15,        //邮件交换
            DNS_QUERY_AAAA=28,      //由域名获取ipv6地址
            DNS_QUERY_AXFR=252,     //传送整个区的请求
            DNS_QUERY_ANY=255,      //对所有记录的请求
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
            std::string m_qname;
            unsigned short m_type = 0;
            unsigned short m_class = htons(1); // 一般为1
        };

        struct Dns_Answer
        {
            std::string m_aname;
            unsigned short m_type = 0;
            unsigned short m_class = htons(1);
            unsigned int m_ttl = 0;
            unsigned short m_data_len = 0;
            std::string m_data;
        };

        class Dns_Protocol
        {
        public:
            Dns_Header& get_header();

            std::queue<Dns_Question>& get_question_queue();

            std::queue<Dns_Answer>& get_answer_queue();

        protected:
            Dns_Header m_header{0};
            std::queue<Dns_Question> m_questions;
            std::queue<Dns_Answer> m_answers;
        };


        class Dns_Request: public Dns_Protocol , public Udp_Request_Base,public Udp_Response_Base
        {
        public:
            using ptr = std::shared_ptr<Dns_Request>;
            //ignore
            int decode(const std::string& buffer) override {return 0;}

            std::string encode() override;
        protected:
            std::string modify_hostname(std::string hostname);
        };


        class Dns_Response: public Dns_Protocol,public Udp_Request_Base,public Udp_Response_Base
        {
        public:
            using ptr = std::shared_ptr<Dns_Response>;
            //ignore
            std::string encode() override {return "";}


            int decode(const std::string& buffer) override;

        protected:
            static bool is_pointer(int in);

            std::string deal_answer(unsigned short type , char *buffer , std::string data);

            int dns_parse_name(unsigned char *buffer, unsigned char *ptr , std::string& out)
            {

                int flag = 0, n = 0, alen = 0;
                char temp[64];
                while (1)
                {

                    flag = static_cast<int>(ptr[0]);
                    if (flag == 0)
                        break;

                    if (is_pointer(flag))
                    {
                        alen += 1;
                        n = (int)ptr[1];
                        ptr = buffer + n;
                        dns_parse_name(buffer, ptr,out);
                        break;
                    }
                    else{
                        bzero(temp,64);
                        ptr++;
                        memcpy(temp,ptr,flag);
                        ptr += flag;
                        alen += (flag + 1); 
                        out += std::string(temp,flag);
                        if(static_cast<int>(ptr[0]) != 0){
                            out += '.';
                        }
                    }
                }
                return alen + 1;
            }
        };
    }
}


#endif