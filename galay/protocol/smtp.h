#ifndef GALAY_SMTP_H
#define GALAY_SMTP_H

#include "../kernel/error.h"
#include "basic_protocol.h"
#include "../security/base64.h"
#include <queue>

namespace galay
{
    namespace protocol{
        class Smtp_Protocol: public Tcp_Request_Base,public Tcp_Response_Base
        {
        public:
            using ptr = std::shared_ptr<Smtp_Protocol>;
            virtual int decode(const std::string &buffer, int &state) override;
            virtual int proto_type() {  return 0;   };
            virtual int proto_fixed_len() { return 0;   };
            virtual int proto_extra_len() { return 0;   };
            virtual void set_extra_msg(std::string &&msg) {};

            virtual std::string encode() override;

            std::string& get_content();

        protected:    
            std::string m_content;
        };

        class Smtp_Request
        {
        public:
            Smtp_Request();
            
            Smtp_Protocol::ptr hello();

            Smtp_Protocol::ptr auth();

            Smtp_Protocol::ptr account(std::string t_account);

            Smtp_Protocol::ptr password(std::string t_password);

            Smtp_Protocol::ptr mailfrom(std::string from_mail);

            Smtp_Protocol::ptr rcptto(std::string to_mail);

            Smtp_Protocol::ptr data();
            
            Smtp_Protocol::ptr msg(std::string subject, std::string content,std::string content_type = "text/html", std::string charset = "utf8mb4");

            Smtp_Protocol::ptr quit();

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

            Smtp_Protocol::ptr resp(){
                return m_smtp_str;
            }

        private:
            Smtp_Protocol::ptr m_smtp_str;
        };
    }
}


#endif