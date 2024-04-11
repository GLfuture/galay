#ifndef GALAY_SMTP_H
#define GALAY_SMTP_H

#include "../kernel/error.h"
#include "basic_protocol.h"
#include "../security/base64.h"
#include <queue>

namespace galay
{
    namespace Protocol{
        class Smtp_Protocol: public Tcp_Request_Base,public Tcp_Response_Base
        {
        public:
            using ptr = std::shared_ptr<Smtp_Protocol>;
            virtual int DecodePdu(const std::string &buffer) override;
            virtual std::string EncodePdu() override;
            virtual Proto_Judge_Type IsPduAndLegal(const std::string& buffer) override;
            std::string& GetContent();

        protected:    
            std::string m_content;
        };

        class Smtp_Request
        {
        public:
            Smtp_Request();
            
            Smtp_Protocol::ptr Hello();

            Smtp_Protocol::ptr Auth();

            Smtp_Protocol::ptr Account(std::string account);

            Smtp_Protocol::ptr Password(std::string password);

            Smtp_Protocol::ptr MailFrom(std::string from_mail);

            Smtp_Protocol::ptr RcptTo(std::string to_mail);

            Smtp_Protocol::ptr Data();
            
            Smtp_Protocol::ptr Msg(std::string subject, std::string content,std::string content_type = "text/html", std::string charset = "utf8mb4");

            Smtp_Protocol::ptr Quit();

        private:
            Smtp_Protocol::ptr m_smtp_str;
            std::string m_frommail;
            std::queue<std::string> m_tomails;
        };

        class Smtp_Response
        {
        public:
            Smtp_Response()
            {
                this->m_smtp_str = std::make_shared<Smtp_Protocol>();
            }

            Smtp_Protocol::ptr Resp(){
                return m_smtp_str;
            }

        private:
            Smtp_Protocol::ptr m_smtp_str;
        };
    }
}


#endif