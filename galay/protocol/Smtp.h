#ifndef __GALAY_SMTP_H__
#define __GALAY_SMTP_H__

#include "Protocol.h"
#include "galay/security/Base64.h"
#include <queue>

namespace galay::error
{
    enum SmtpErrorCode
    {
        kSmtpError_NoError = 0,
        kSmtpError_Incomplete,
    };

    class SmtpError
    {
    public:
        using ptr = std::shared_ptr<SmtpError>;
        using wptr = std::weak_ptr<SmtpError>;
        using uptr = std::unique_ptr<SmtpError>;
        bool HasError() const;
        SmtpErrorCode& Code();
        void Reset();
        std::string ToString(SmtpErrorCode code) const;
    protected:
        SmtpErrorCode m_code = kSmtpError_NoError;
    };
}

namespace galay::protocol::smtp
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

    class SmtpRequest: public Request, public common::DynamicCreator<Request,SmtpRequest>
    {
        friend class SmtpHelper;
    public:
        using ptr = std::shared_ptr<SmtpRequest>;
        using wpt = std::weak_ptr<SmtpRequest>;
        using uptr = std::unique_ptr<SmtpRequest>;
        SmtpRequest();
        std::pair<bool,size_t> DecodePdu(const std::string_view &buffer) override;
        std::string EncodePdu() const override;
        bool HasError() const override;
        int GetErrorCode() const override;
        std::string GetErrorString() override;
        void Reset() override;
        std::string& GetContent();
    private:
        //content不带\r\n
        uint32_t m_next_index;
        std::string m_content;
        std::string m_frommail;
        std::queue<std::string> m_tomails;
        error::SmtpError::ptr m_error;
    };

    class SmtpResponse: public Response, public common::DynamicCreator<Response,SmtpResponse>
    {
    public:
        using ptr = std::shared_ptr<SmtpResponse>;
        using wptr = std::weak_ptr<SmtpResponse>;
        using uptr = std::unique_ptr<SmtpResponse>;
        SmtpResponse();
        std::pair<bool,size_t> DecodePdu(const std::string_view &buffer) override;
        std::string EncodePdu() const override;
        bool HasError() const override;
        int GetErrorCode() const override;
        std::string GetErrorString() override;
        void Reset() override;
        std::string& GetContent();
    private:
        uint32_t m_next_index;
        error::SmtpError::ptr m_error;
        std::string m_content;
    };
    }
#endif