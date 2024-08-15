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
            struct SmtpMsgInfo
            {
                std::string m_subject;
                std::string m_content;
                std::string m_content_type = "text/html";
                std::string m_charset = "utf8mb4";
            };

            class SmtpRequest;

            class SmtpHelper
            {
            public:
                static std::string Hello(SmtpRequest& request);
                static std::string Auth(SmtpRequest& request);
                static std::string Account(SmtpRequest& request, std::string account);
                static std::string Password(SmtpRequest& request, std::string password);
                static std::string MailFrom(SmtpRequest& request, std::string from_mail);
                static std::string RcptTo(SmtpRequest& request, std::string to_mail);
                static std::string Data(SmtpRequest& request);
                static std::string Msg(SmtpRequest& request, const SmtpMsgInfo& msg);
                static std::string Quit(SmtpRequest& request);
            };

            class SmtpRequest: public GY_Request, public common::GY_DynamicCreator<GY_Request,SmtpRequest>
            {
                friend class SmtpHelper;
            public:
                using ptr = std::shared_ptr<SmtpRequest>;
                using wpt = std::weak_ptr<SmtpRequest>;
                using uptr = std::unique_ptr<SmtpRequest>;
                SmtpRequest() = default;
                virtual ProtoJudgeType DecodePdu(std::string &buffer) override;
                virtual std::string EncodePdu() override;
                std::string& GetContent();
            private:
                //content不带\r\n
                std::string m_content;
                std::string m_frommail;
                std::queue<std::string> m_tomails;
            };

            class SmtpResponse: public GY_Response, public common::GY_DynamicCreator<GY_Response,SmtpResponse>
            {
            public:
                using ptr = std::shared_ptr<SmtpResponse>;
                using wptr = std::weak_ptr<SmtpResponse>;
                using uptr = std::unique_ptr<SmtpResponse>;
                SmtpResponse() = default;
                virtual ProtoJudgeType DecodePdu(std::string &buffer) override;
                virtual std::string EncodePdu() override;
                std::string& GetContent();
            private:
                std::string m_content;
            };
        }
    }
}

#endif