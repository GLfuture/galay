#include "smtp.h"

std::string 
galay::protocol::smtp::SmtpHelper::Hello(SmtpRequest& request)
{
    request.m_content = "HELO MSG";
    return request.EncodePdu();
}

std::string
galay::protocol::smtp::SmtpHelper::Auth(SmtpRequest& request)
{
    request.m_content = "AUTH LOGIN";
    return request.EncodePdu();
}

std::string
galay::protocol::smtp::SmtpHelper::Account(SmtpRequest& request, std::string account)
{
    request.m_content = security::Base64Util::base64_encode(account);
    return request.EncodePdu();
}

std::string
galay::protocol::smtp::SmtpHelper::Password(SmtpRequest& request, std::string password)
{
    request.m_content = security::Base64Util::base64_encode(password);
    return request.EncodePdu();
}

std::string
galay::protocol::smtp::SmtpHelper::MailFrom(SmtpRequest& request, std::string from_mail)
{
    request.m_frommail = from_mail;
    std::string res = "MAIL FROM: <";
    request.m_content = res + from_mail + '>';
    return request.EncodePdu();
}

std::string
galay::protocol::smtp::SmtpHelper::RcptTo(SmtpRequest& request, std::string to_mail)
{
    request.m_tomails.push(to_mail);
    std::string res = "RCPT TO: <";
    request.m_content = res + to_mail + '>';
    return request.EncodePdu();
}

std::string
galay::protocol::smtp::SmtpHelper::Data(SmtpRequest& request)
{
    request.m_content = "DATA";
    return request.EncodePdu();
}

std::string
galay::protocol::smtp::SmtpHelper::Msg(SmtpRequest& request, const SmtpMsgInfo& msg)
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
galay::protocol::smtp::SmtpHelper::Quit(SmtpRequest& request)
{
    request.m_content = "QUIT\r\n";
    return request.EncodePdu();
}

galay::protocol::ProtoType 
galay::protocol::smtp::SmtpRequest::DecodePdu(std::string& buffer)
{
    int pos = buffer.find("\r\n");
    if(pos == std::string::npos) return protocol::ProtoType::kProtoIncomplete;
    this->m_content = buffer.substr(0,pos);
    buffer.erase(0,pos + 2);
    return protocol::ProtoType::kProtoFinished;
}

std::string 
galay::protocol::smtp::SmtpRequest::EncodePdu()
{
    return this->m_content + "\r\n";
}

std::string& 
galay::protocol::smtp::SmtpRequest::GetContent()
{
    return this->m_content;
}


galay::protocol::ProtoType 
galay::protocol::smtp::SmtpResponse::DecodePdu(std::string &buffer)
{
    int pos = buffer.find("\r\n");
    if(pos == std::string::npos) return protocol::ProtoType::kProtoIncomplete;
    this->m_content = buffer.substr(0,pos);
    buffer.erase(0,pos + 2);
    return protocol::ProtoType::kProtoFinished;
}


std::string 
galay::protocol::smtp::SmtpResponse::EncodePdu()
{
    return this->m_content + "\r\n";
}


std::string& 
galay::protocol::smtp::SmtpResponse::GetContent()
{
    return this->m_content;
}