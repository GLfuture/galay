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
            class SmtpProtocol : public GY_SRequest, public GY_SResponse
                ,public galay::common::GY_DynamicCreator<GY_SRequest,SmtpProtocol>,public galay::common::GY_DynamicCreator<GY_SResponse,SmtpProtocol>
            {
            public:
                using ptr = std::shared_ptr<SmtpProtocol>;
                virtual std::string EncodePdu() override;
                virtual galay::protocol::ProtoJudgeType DecodePdu(std::string &buffer) override;
                void SetContent(std::string content);
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

            class SmtpRequest
            {
            public:
                using ptr = std::shared_ptr<SmtpRequest>;
                using wpt = std::weak_ptr<SmtpRequest>;
                using uptr = std::unique_ptr<SmtpRequest>;
                SmtpRequest();
                SmtpProtocol::ptr Hello();
                SmtpProtocol::ptr Auth();
                SmtpProtocol::ptr Account(std::string account);
                SmtpProtocol::ptr Password(std::string password);
                SmtpProtocol::ptr MailFrom(std::string from_mail);
                SmtpProtocol::ptr RcptTo(std::string to_mail);
                SmtpProtocol::ptr Data();
                SmtpProtocol::ptr Msg(const SmtpMsgInfo& msg);
                SmtpProtocol::ptr Quit();

            private:
                SmtpProtocol::ptr m_smtp_str;
                std::string m_frommail;
                std::queue<std::string> m_tomails;
            };

            class SmtpResponse
            {
            public:
                using ptr = std::shared_ptr<SmtpResponse>;
                using wptr = std::weak_ptr<SmtpResponse>;
                using uptr = std::unique_ptr<SmtpResponse>;
                SmtpResponse();
                SmtpProtocol::ptr Resp();
            private:
                SmtpProtocol::ptr m_smtp_str;
            };
        }
    }
}

#endif