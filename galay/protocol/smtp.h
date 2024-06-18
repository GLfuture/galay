#ifndef GALAY_SMTP_H
#define GALAY_SMTP_H

#include "basic_protocol.h"
#include "../security/base64.h"
#include <queue>

namespace galay
{
    namespace protocol
    {
        namespace smtp
        {
            class Smtp_Protocol : public GY_TcpRequest, public GY_TcpResponse
            {
            public:
                using ptr = std::shared_ptr<Smtp_Protocol>;
                virtual std::string EncodePdu() override;
                virtual ProtoJudgeType DecodePdu(std::string &buffer) override;
                std::string GetContent();
                void SetContent(std::string content);
                virtual void Clear() override;

            protected:
                std::string m_content;
            };

            struct SmtpMsgInfo
            {
                std::string m_subject;
                std::string m_content;
                std::string m_content_type = "text/html";
                std::string m_charset = "utf8mb4";
            };

            class Smtp_Request
            {
            public:
                using ptr = std::shared_ptr<Smtp_Request>;
                using wpt = std::weak_ptr<Smtp_Request>;
                using uptr = std::unique_ptr<Smtp_Request>;
                Smtp_Request();
                Smtp_Protocol::ptr Hello();
                Smtp_Protocol::ptr Auth();
                Smtp_Protocol::ptr Account(std::string account);
                Smtp_Protocol::ptr Password(std::string password);
                Smtp_Protocol::ptr MailFrom(std::string from_mail);
                Smtp_Protocol::ptr RcptTo(std::string to_mail);
                Smtp_Protocol::ptr Data();
                Smtp_Protocol::ptr Msg(const SmtpMsgInfo& msg);
                Smtp_Protocol::ptr Quit();

            private:
                Smtp_Protocol::ptr m_smtp_str;
                std::string m_frommail;
                std::queue<std::string> m_tomails;
            };

            class Smtp_Response
            {
            public:
                using ptr = std::shared_ptr<Smtp_Response>;
                using wptr = std::weak_ptr<Smtp_Response>;
                using uptr = std::unique_ptr<Smtp_Response>;
                Smtp_Response();
                Smtp_Protocol::ptr Resp();
            private:
                Smtp_Protocol::ptr m_smtp_str;
            };
        }
    }
}

#endif