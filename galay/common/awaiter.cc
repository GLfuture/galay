#include "awaiter.h"
#include "coroutine.h"
galay::common::CommonAwaiter::CommonAwaiter(bool IsSuspend,const std::any& result)
{
    m_IsSuspend = IsSuspend;
    m_Result = result;
}

galay::common::CommonAwaiter::CommonAwaiter(CommonAwaiter&& other)
{
    if(this != &other){
        m_IsSuspend = other.m_IsSuspend;
        m_Result = std::move(other.m_Result);
        other.m_Result.reset();
    }
}

galay::common::CommonAwaiter&
galay::common::CommonAwaiter::operator=(CommonAwaiter&& other)
{
    if(this != &other){
        m_IsSuspend = other.m_IsSuspend;
        m_Result = std::move(other.m_Result);
        other.m_Result.reset();
    }
    return *this;
}

bool 
galay::common::CommonAwaiter::await_ready()
{
    return !m_IsSuspend;
}

void 
galay::common::CommonAwaiter::await_suspend(std::coroutine_handle<> handle) 
{
    
}

std::any 
galay::common::CommonAwaiter::await_resume()
{
    return m_Result;
}

galay::common::GroupAwaiter::GroupAwaiter(bool IsSuspend)
{
    this->m_IsSuspend = IsSuspend;
}

galay::common::GroupAwaiter&
galay::common::GroupAwaiter::operator=(const GroupAwaiter& other)
{
    if(this != &other){
        this->m_coId=other.m_coId;
        this->m_IsSuspend.store(other.m_IsSuspend.load());
    }
    return *this;
}

galay::common::GroupAwaiter::GroupAwaiter(GroupAwaiter&& other)
{
    this->m_coId=other.m_coId;
    this->m_IsSuspend.store(other.m_IsSuspend.load());
}

galay::common::GroupAwaiter&
galay::common::GroupAwaiter::operator=(GroupAwaiter&& other)
{
    if(this != &other){
        this->m_coId=other.m_coId;
        this->m_IsSuspend.store(other.m_IsSuspend.load());
    }
    return *this;
}

bool 
galay::common::GroupAwaiter::await_ready()
{
    return !m_IsSuspend.load(); 
}

void 
galay::common::GroupAwaiter::Resume()
{
    if(this->m_IsSuspend) {
        GY_NetCoroutinePool::GetInstance()->Resume(this->m_coId);
        this->m_IsSuspend = false;
    }
}

void 
galay::common::GroupAwaiter::await_suspend(std::coroutine_handle<> handle)
{
    this->m_coId = reinterpret_cast<uint64_t>(handle.address());
}

std::any 
galay::common::GroupAwaiter::await_resume()
{
    return {};
}

