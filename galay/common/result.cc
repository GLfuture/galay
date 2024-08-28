#include "result.h"
#include <spdlog/spdlog.h>

namespace galay::result
{
ResultInterface::ResultInterface()
{
    m_success = false;
    m_errMsg = "";
    m_waitGroup = std::make_shared<coroutine::WaitGroup>();
}

void 
ResultInterface::Done()
{
    m_waitGroup->Done();
}


bool 
ResultInterface::Success()
{
    return m_success;
}

std::any 
ResultInterface::Result()
{
    return m_result;
}

std::string
ResultInterface::Error()
{
    return m_errMsg;
}

void 
ResultInterface::AddTaskNum(uint16_t taskNum)
{
    m_waitGroup->Add(taskNum);
}

galay::coroutine::GroupAwaiter&
ResultInterface::Wait()
{
    return m_waitGroup->Wait();
}

void 
ResultInterface::SetResult(std::any result)
{
    m_result = result;
}

void 
ResultInterface::SetErrorMsg(std::string errMsg)
{
    m_errMsg = errMsg;
}

void 
ResultInterface::SetSuccess(bool success)
{
    m_success = success;
    if(success)
    {
        if(!m_errMsg.empty()) m_errMsg.clear();
    }
}

}



