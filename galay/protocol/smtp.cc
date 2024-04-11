#include "smtp.h"

galay::Proto_Judge_Type 
galay::Protocol::Smtp_Protocol:: IsPduAndLegal(const std::string& buffer)
{
    if(buffer.find("\r\n")) return Proto_Judge_Type::PROTOCOL_LEGAL;
    return Proto_Judge_Type::PROTOCOL_INCOMPLETE;
}

int 
galay::Protocol::Smtp_Protocol::DecodePdu(const std::string &buffer)
{
    this->m_content = buffer.substr(0,buffer.find("\r\n"));
    return this->m_content.length() + 2;
}

std::string 
galay::Protocol::Smtp_Protocol::EncodePdu()
{
    return this->m_content + "\r\n";
}

std::string&
galay::Protocol::Smtp_Protocol::GetContent()
{
    return this->m_content;
}

galay::Protocol::Smtp_Request::Smtp_Request()
{
    m_smtp_str = std::make_shared<Smtp_Protocol>();
}

galay::Protocol::Smtp_Protocol::ptr 
galay::Protocol::Smtp_Request::Hello()
{
    m_smtp_str->GetContent() = "HELO MSG";
    return m_smtp_str;
}

galay::Protocol::Smtp_Protocol::ptr 
galay::Protocol::Smtp_Request::Auth()
{
    m_smtp_str->GetContent() = "AUTH LOGIN";
    return m_smtp_str;
}

galay::Protocol::Smtp_Protocol::ptr 
galay::Protocol::Smtp_Request::Account(std::string t_account)
{
    m_smtp_str->GetContent() = Security::Base64Util::base64_encode(t_account);
    return m_smtp_str;
}

galay::Protocol::Smtp_Protocol::ptr 
galay::Protocol::Smtp_Request::Password(std::string t_password)
{
    m_smtp_str->GetContent() = Security::Base64Util::base64_encode(t_password);
    return m_smtp_str;
}

galay::Protocol::Smtp_Protocol::ptr 
galay::Protocol::Smtp_Request::MailFrom(std::string from_mail)
{
    m_frommail = from_mail;
    std::string res = "MAIL FROM: <";
    m_smtp_str->GetContent() = res + from_mail + ">";
    return m_smtp_str;
}

galay::Protocol::Smtp_Protocol::ptr 
galay::Protocol::Smtp_Request::RcptTo(std::string to_mail)
{
    m_tomails.push(to_mail);
    std::string res = "RCPT TO: <";
    m_smtp_str->GetContent() = res + to_mail + ">";
    return m_smtp_str;
}

galay::Protocol::Smtp_Protocol::ptr 
galay::Protocol::Smtp_Request::Data()
{
    m_smtp_str->GetContent() = "DATA";
    return m_smtp_str;
}

galay::Protocol::Smtp_Protocol::ptr 
galay::Protocol::Smtp_Request::Msg(std::string subject, std::string content, std::string content_type, std::string charset)
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
    res = res + "Subject: " + subject + "\r\nContent-Type: " + content_type + ";charset=" + charset + "\r\n\r\n" + content + "\r\n\r\n.\r\n";
    m_smtp_str->GetContent() = res;
    return m_smtp_str;
}

galay::Protocol::Smtp_Protocol::ptr 
galay::Protocol::Smtp_Request::Quit()
{
    m_smtp_str->GetContent() = "QUIT\r\n";
    return m_smtp_str;
}