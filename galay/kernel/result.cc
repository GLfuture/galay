#include "result.h"
#include "../common/coroutine.h"
#include "../common/waitgroup.h"

galay::kernel::NetResult::NetResult()
{
    m_success = false;
    m_errMsg = "";
    m_waitGroup = std::make_shared<coroutine::WaitGroup>();
}

bool 
galay::kernel::NetResult::Success()
{
    return m_success;
}

std::string
galay::kernel::NetResult::Error()
{
    return m_errMsg;
}

void 
galay::kernel::NetResult::AddTaskNum(uint16_t taskNum)
{
    m_waitGroup->Add(taskNum);
}

galay::coroutine::GroupAwaiter&
galay::kernel::NetResult::Wait()
{
    return m_waitGroup->Wait();
}

void 
galay::kernel::NetResult::Done()
{
    m_waitGroup->Done();
}


galay::kernel::HttpResult::HttpResult()
{
}

galay::protocol::http::HttpResponse 
galay::kernel::HttpResult::GetResponse()
{
    std::string respStr = std::any_cast<std::string>(this->m_result);
    protocol::http::HttpResponse response;
    response.DecodePdu(respStr);
    return response;
}

galay::kernel::SmtpResult::SmtpResult()
{
}

std::queue<galay::protocol::smtp::SmtpResponse> 
galay::kernel::SmtpResult::GetResponse()
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


galay::kernel::DnsResult::DnsResult()
{
    m_isParsed = false;
}

bool 
galay::kernel::DnsResult::HasCName()
{
    if(!m_isParsed) {
        Parse();
        m_isParsed = true;
    }
    return !m_cNames.empty();
}

std::string 
galay::kernel::DnsResult::GetCName()
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
galay::kernel::DnsResult::HasA()
{
    if(!m_isParsed) {
        Parse();
        m_isParsed = true;
    }
    return !m_a.empty();
}

std::string 
galay::kernel::DnsResult::GetA()
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
galay::kernel::DnsResult::HasAAAA()
{
    if(!m_isParsed) {
        Parse();
        m_isParsed = true;
    }
    return !m_aaaa.empty();
}

std::string 
galay::kernel::DnsResult::GetAAAA()
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
galay::kernel::DnsResult::HasPtr()
{
    if(!m_isParsed) {
        Parse();
        m_isParsed = true;
    }
    return !m_ptr.empty();
}

std::string 
galay::kernel::DnsResult::GetPtr()
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
galay::kernel::DnsResult::Parse()
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
