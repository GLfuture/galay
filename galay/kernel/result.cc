#include "result.h"
#include "../common/coroutine.h"
#include "../common/waitgroup.h"

namespace galay::result
{
NetResult::NetResult()
{
    m_success = false;
    m_errMsg = "";
    m_waitGroup = std::make_shared<coroutine::WaitGroup>();
}

bool 
NetResult::Success()
{
    return m_success;
}

std::any 
NetResult::Result()
{
    return m_result;
}

std::string
NetResult::Error()
{
    return m_errMsg;
}

void 
NetResult::AddTaskNum(uint16_t taskNum)
{
    m_waitGroup->Add(taskNum);
}

galay::coroutine::GroupAwaiter&
NetResult::Wait()
{
    return m_waitGroup->Wait();
}

void 
NetResultInner::SetResult(std::any result)
{
    m_result = result;
}

void 
NetResultInner::SetErrorMsg(std::string errMsg)
{
    m_errMsg = errMsg;
}

void 
NetResultInner::SetSuccess(bool success)
{
    m_success = success;
    if(success)
    {
        if(!m_errMsg.empty()) m_errMsg.clear();
    }
}

void 
NetResult::Done()
{
    m_waitGroup->Done();
}


HttpResult::HttpResult()
{
}

galay::protocol::http::HttpResponse 
HttpResult::GetResponse()
{
    std::string respStr = std::any_cast<std::string>(this->m_result);
    protocol::http::HttpResponse response;
    response.DecodePdu(respStr);
    return response;
}

SmtpResult::SmtpResult()
{
}

std::queue<galay::protocol::smtp::SmtpResponse> 
SmtpResult::GetResponse()
{
    std::queue<galay::protocol::smtp::SmtpResponse> smtpResponses;
    std::queue<std::string> responses = std::any_cast<std::queue<std::string>>(this->m_result);
    while(!responses.empty())
    {
        auto response = responses.front();
        responses.pop();
        protocol::smtp::SmtpResponse smtp;
        smtp.DecodePdu(response);
        smtpResponses.push(smtp);
    }
    return smtpResponses;
}


DnsResult::DnsResult()
{
    m_isParsed = false;
}

bool 
DnsResult::HasCName()
{
    if(!m_isParsed) {
        Parse();
        m_isParsed = true;
    }
    return !m_cNames.empty();
}

std::string 
DnsResult::GetCName()
{
    if(!m_isParsed) {
        Parse();
        m_isParsed = true;
    }
    std::string cname = m_cNames.front();
    m_cNames.pop();
    return std::move(cname);
}

bool 
DnsResult::HasA()
{
    if(!m_isParsed) {
        Parse();
        m_isParsed = true;
    }
    return !m_a.empty();
}

std::string 
DnsResult::GetA()
{
    if(!m_isParsed) {
        Parse();
        m_isParsed = true;
    }
    std::string a = m_a.front();
    m_a.pop();
    return std::move(a);
}

bool 
DnsResult::HasAAAA()
{
    if(!m_isParsed) {
        Parse();
        m_isParsed = true;
    }
    return !m_aaaa.empty();
}

std::string 
DnsResult::GetAAAA()
{
    if(!m_isParsed) {
        Parse();
        m_isParsed = true;
    }
    std::string aaaa = m_aaaa.front();
    m_aaaa.pop();
    return std::move(aaaa);
}

bool 
DnsResult::HasPtr()
{
    if(!m_isParsed) {
        Parse();
        m_isParsed = true;
    }
    return !m_ptr.empty();
}

std::string 
DnsResult::GetPtr()
{
    if(!m_isParsed) {
        Parse();
        m_isParsed = true;
    }
    std::string ptr = m_ptr.front();
    m_ptr.pop();
    return std::move(ptr);
}

void
DnsResult::Parse()
{
    UdpResInfo info = std::any_cast<UdpResInfo>(this->m_result);
    protocol::dns::DnsResponse response;
    response.DecodePdu(info.m_buffer);
    auto answers = response.GetAnswerQueue();
    while (!answers.empty())
    {
        auto answer = answers.front();
        answers.pop();
        spdlog::debug("[{}:{}] DnsResult::Parse: type:{}, data:{}",__FILE__, __LINE__, answer.m_type, answer.m_data);
        switch (answer.m_type)
        {
        case protocol::dns::kDnsQueryCname:
            m_cNames.push(answer.m_data);
            break;
        case protocol::dns::kDnsQueryA:
            m_a.push(answer.m_data);
            break;
        case protocol::dns::kDnsQueryAAAA:
            m_aaaa.push(answer.m_data);
            break;
        case protocol::dns::kDnsQueryPtr:
            m_ptr.push(answer.m_data);
            break;
        default:
            break;
        }
    }
    
}

}



