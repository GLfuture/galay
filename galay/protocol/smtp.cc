#include "smtp.h"

galay::ProtoJudgeType 
galay::protocol::smtp::Smtp_Protocol:: DecodePdu(std::string& buffer)
{
    int pos = buffer.find("\r\n");
    if(pos == std::string::npos) return ProtoJudgeType::kProtoIncomplete;
    this->m_content = buffer.substr(0,pos);
    buffer.erase(0,pos + 2);
    return ProtoJudgeType::kProtoFinished;
}

std::string 
galay::protocol::smtp::Smtp_Protocol::EncodePdu()
{
    return this->m_content;
}

void 
galay::protocol::smtp::Smtp_Protocol::Clear()
{
    this->m_content.clear();
}

std::string
galay::protocol::smtp::Smtp_Protocol::GetContent()
{
    return this->m_content;
}

void 
galay::protocol::smtp::Smtp_Protocol::SetContent(std::string content)
{
    this->m_content = content;
}

galay::protocol::smtp::Smtp_Request::Smtp_Request()
{
    m_smtp_str = std::make_shared<Smtp_Protocol>();
}

galay::protocol::smtp::Smtp_Protocol::ptr 
galay::protocol::smtp::Smtp_Request::Hello()
{
    m_smtp_str->SetContent("HELO MSG\r\n");
    return m_smtp_str;
}

galay::protocol::smtp::Smtp_Protocol::ptr 
galay::protocol::smtp::Smtp_Request::Auth()
{
    m_smtp_str->SetContent("AUTH LOGIN\r\n");
    return m_smtp_str;
}

galay::protocol::smtp::Smtp_Protocol::ptr 
galay::protocol::smtp::Smtp_Request::Account(std::string account)
{
    m_smtp_str->SetContent(security::Base64Util::base64_encode(account) + "\r\n");
    return m_smtp_str;
}

galay::protocol::smtp::Smtp_Protocol::ptr 
galay::protocol::smtp::Smtp_Request::Password(std::string password)
{
    m_smtp_str->SetContent(security::Base64Util::base64_encode(password) + "\r\n");
    return m_smtp_str;
}

galay::protocol::smtp::Smtp_Protocol::ptr 
galay::protocol::smtp::Smtp_Request::MailFrom(std::string from_mail)
{
    m_frommail = from_mail;
    std::string res = "MAIL FROM: <";
    m_smtp_str->SetContent(res + from_mail + ">\r\n");
    return m_smtp_str;
}

galay::protocol::smtp::Smtp_Protocol::ptr 
galay::protocol::smtp::Smtp_Request::RcptTo(std::string to_mail)
{
    m_tomails.push(to_mail);
    std::string res = "RCPT TO: <";
    m_smtp_str->SetContent(res + to_mail + ">\r\n");
    return m_smtp_str;
}

galay::protocol::smtp::Smtp_Protocol::ptr 
galay::protocol::smtp::Smtp_Request::Data()
{
    m_smtp_str->SetContent("DATA\r\n");
    return m_smtp_str;
}

galay::protocol::smtp::Smtp_Protocol::ptr 
galay::protocol::smtp::Smtp_Request::Msg(const SmtpMsgInfo& msg)
{
    std::string res = "From: <";
    res += m_frommail + ">\r\nTo: ";
    while (!m_tomails.empty())
    {
        res = res + "<" + m_tomails.front() + ">";
        m_tomails.pop();
        if (!m_tomails.empty())
        {
            res += ", ";
        }
        else
        {
            res += "\r\n";
        }
    }
    res = res + "Subject: " + msg.m_subject + "\r\nContent-Type: " + msg.m_content_type +\
        ";charset=" + msg.m_charset + "\r\n\r\n" + msg.m_content + "\r\n\r\n.\r\n\r\n";
    m_smtp_str->SetContent(res);
    return m_smtp_str;
}

galay::protocol::smtp::Smtp_Protocol::ptr 
galay::protocol::smtp::Smtp_Request::Quit()
{
    m_smtp_str->SetContent("QUIT\r\n\r\n");
    return m_smtp_str;
}

galay::protocol::smtp::Smtp_Response::Smtp_Response()
{
    this->m_smtp_str = std::make_shared<Smtp_Protocol>();
}

galay::protocol::smtp::Smtp_Protocol::ptr
galay::protocol::smtp::Smtp_Response::Resp()
{
    return m_smtp_str;
}