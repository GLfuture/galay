#include "Smtp.h"

namespace galay::error
{

static const char* SmtpErrors[] = {
    "NoError",
    "InComplete",
};
    
bool SmtpError::HasError() const
{
    return m_code != kSmtpError_NoError;
}

SmtpErrorCode &SmtpError::Code()
{
    return m_code;
}

void SmtpError::Reset()
{
    m_code = kSmtpError_NoError;
}

std::string SmtpError::ToString(SmtpErrorCode code) const
{
    return SmtpErrors[code];
}
}

namespace galay::protocol::smtp
{
std::string 
SmtpHelper::Hello(SmtpRequest& request)
{
    request.m_content = "HELO MSG";
    return request.EncodePdu();
}

std::string
SmtpHelper::Auth(SmtpRequest& request)
{
    request.m_content = "AUTH LOGIN";
    return request.EncodePdu();
}

std::string
SmtpHelper::Account(SmtpRequest& request, std::string account)
{
    request.m_content = security::Base64Util::Base64Encode(account);
    return request.EncodePdu();
}

std::string
SmtpHelper::Password(SmtpRequest& request, std::string password)
{
    request.m_content = security::Base64Util::Base64Encode(password);
    return request.EncodePdu();
}

std::string
SmtpHelper::MailFrom(SmtpRequest& request, std::string from_mail)
{
    request.m_frommail = from_mail;
    std::string res = "MAIL FROM: <";
    request.m_content = res + from_mail + '>';
    return request.EncodePdu();
}

std::string
SmtpHelper::RcptTo(SmtpRequest& request, std::string to_mail)
{
    request.m_tomails.push(to_mail);
    std::string res = "RCPT TO: <";
    request.m_content = res + to_mail + '>';
    return request.EncodePdu();
}

std::string
SmtpHelper::Data(SmtpRequest& request)
{
    request.m_content = "DATA";
    return request.EncodePdu();
}

std::string
SmtpHelper::Msg(SmtpRequest& request, const SmtpMsgInfo& msg)
{
    std::string res = "From: <";
    res += request.m_frommail + ">\r\nTo: ";
    request.m_frommail = "";
    while (!request.m_tomails.empty())
    {
        res = res + "<" + request.m_tomails.front() + ">";
        request.m_tomails.pop();
        if (!request.m_tomails.empty())
        {
            res += ", ";
        }
        else
        {
            res += "\r\n";
        }
    }
    res = res + "Subject: " + msg.m_subject + "\r\nContent-Type: " + msg.m_content_type +\
        ";charset=" + msg.m_charset + "\r\n\r\n" + msg.m_content + "\r\n\r\n.\r\n";
    request.m_content = std::move(res);
    return request.EncodePdu();
}

std::string
SmtpHelper::Quit(SmtpRequest& request)
{
    request.m_content = "QUIT\r\n";
    return request.EncodePdu();
}

SmtpRequest::SmtpRequest()
{
    m_error = std::make_shared<error::SmtpError>();    
}

int 
SmtpRequest::DecodePdu(const std::string_view& buffer)
{
    m_error->Reset();
    int pos = buffer.find("\r\n");
    if(pos == std::string::npos) {
        m_error->Code() = error::kSmtpError_Incomplete;
        return 0;
    }
    this->m_content = buffer.substr(0,pos);
    return pos + 2;
}

std::string 
SmtpRequest::EncodePdu()
{
    return this->m_content + "\r\n";
}

bool SmtpRequest::HasError() const
{
    return m_error->HasError();
}

int protocol::smtp::SmtpRequest::GetErrorCode() const
{
    return m_error->Code();
}

std::string protocol::smtp::SmtpRequest::GetErrorString()
{
    return m_error->ToString(m_error->Code());
}

void protocol::smtp::SmtpRequest::Reset()
{
    m_error->Reset();
    m_content.clear();
    m_frommail.clear();
    while(!m_tomails.empty()){
        m_tomails.pop();
    }
}

std::string& 
SmtpRequest::GetContent()
{
    return this->m_content;
}

SmtpResponse::SmtpResponse()
{
    m_error = std::make_shared<error::SmtpError>();
}

int 
SmtpResponse::DecodePdu(const std::string_view &buffer)
{
    m_error->Reset();
    int pos = buffer.find("\r\n");
    if(pos == std::string::npos) {
        m_error->Code() = error::kSmtpError_Incomplete;
        return 0;
    }
    this->m_content = buffer.substr(0,pos);
    return pos + 2;
}


std::string 
SmtpResponse::EncodePdu()
{
    return this->m_content + "\r\n";
}

bool protocol::smtp::SmtpResponse::HasError() const
{
    return m_error->HasError();
}

int protocol::smtp::SmtpResponse::GetErrorCode() const
{
    return m_error->Code();
}

std::string protocol::smtp::SmtpResponse::GetErrorString()
{
    return m_error->ToString(m_error->Code());
}

void protocol::smtp::SmtpResponse::Reset()
{
    m_error->Reset();
    m_content.clear();
}

std::string& 
SmtpResponse::GetContent()
{
    return this->m_content;
}

}


