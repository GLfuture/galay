#include "result.h"
#include "../common/coroutine.h"
#include "../common/waitgroup.h"

galay::kernel::TcpResult::TcpResult()
{
    m_success = false;
    m_errMsg = "";
    m_waitGroup = std::make_shared<common::WaitGroup>();
}

bool 
galay::kernel::TcpResult::Success()
{
    return m_success;
}

std::string
galay::kernel::TcpResult::Error()
{
    return m_errMsg;
}

void 
galay::kernel::TcpResult::AddTaskNum(uint16_t taskNum)
{
    m_waitGroup->Add(taskNum);
}

galay::common::GroupAwaiter&
galay::kernel::TcpResult::Wait()
{
    return m_waitGroup->Wait();
}

void 
galay::kernel::TcpResult::Done()
{
    m_waitGroup->Done();
}


galay::kernel::HttpResult::HttpResult()
{
}

galay::protocol::http::HttpResponse 
galay::kernel::HttpResult::GetResponse()
{
    std::string respStr = std::any_cast<std::string>(this->m_resp);
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
    std::queue<std::string> responses = std::any_cast<std::queue<std::string>>(this->m_resp);
    while(!responses.empty())
    {
        auto response = responses.front();
        responses.pop();
        protocol::smtp::SmtpResponse smtp;
        smtp.Resp()->DecodePdu(response);
        smtpResponses.push(smtp);
    }
    return smtpResponses;
}
