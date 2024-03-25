#include "smtp.h"

int galay::protocol::Smtp_Protocol::decode(const std::string &buffer, int &state)
{
    this->m_content = buffer;
    state = Error::NoError::GY_SUCCESS;
    return 0;
}

std::string galay::protocol::Smtp_Protocol::encode()
{
    return this->m_content;
}

std::string &galay::protocol::Smtp_Protocol::get_content()
{
    return this->m_content;
}

galay::protocol::Smtp_Request::Smtp_Request()
{
    m_smtp_str = std::make_shared<Smtp_Protocol>();
}

galay::protocol::Smtp_Protocol::ptr galay::protocol::Smtp_Request::hello()
{
    m_smtp_str->get_content() = "HELO MSG\r\n";
    return m_smtp_str;
}

galay::protocol::Smtp_Protocol::ptr galay::protocol::Smtp_Request::auth()
{
    m_smtp_str->get_content() = "AUTH LOGIN\r\n";
    return m_smtp_str;
}

galay::protocol::Smtp_Protocol::ptr galay::protocol::Smtp_Request::account(std::string t_account)
{
    m_smtp_str->get_content() = Base64Util::base64_encode(t_account) + "\r\n";
    return m_smtp_str;
}

galay::protocol::Smtp_Protocol::ptr galay::protocol::Smtp_Request::password(std::string t_password)
{
    m_smtp_str->get_content() = Base64Util::base64_encode(t_password)+ "\r\n";
    return m_smtp_str;
}

galay::protocol::Smtp_Protocol::ptr galay::protocol::Smtp_Request::mailfrom(std::string from_mail)
{
    m_frommail = from_mail;
    std::string res = "MAIL FROM: <";
    m_smtp_str->get_content() = res + from_mail + ">\r\n";
    return m_smtp_str;
}

galay::protocol::Smtp_Protocol::ptr galay::protocol::Smtp_Request::rcptto(std::string to_mail)
{
    m_tomails.push(to_mail);
    std::string res = "RCPT TO: <";
    m_smtp_str->get_content() = res + to_mail + ">\r\n";
    return m_smtp_str;
}

galay::protocol::Smtp_Protocol::ptr galay::protocol::Smtp_Request::data()
{
    m_smtp_str->get_content() = "DATA\r\n";
    return m_smtp_str;
}

galay::protocol::Smtp_Protocol::ptr galay::protocol::Smtp_Request::msg(std::string subject, std::string content, std::string content_type, std::string charset)
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
    res = res + "Subject: " + subject + "\r\nContent-Type: " + content_type + ";charset=" + charset + "\r\n\r\n" + content + "\r\n\r\n.\r\n\r\n";
    m_smtp_str->get_content() = res;
    return m_smtp_str;
}

galay::protocol::Smtp_Protocol::ptr galay::protocol::Smtp_Request::quit()
{
    m_smtp_str->get_content() = "QUIT\r\n\r\n";
    return m_smtp_str;
}