#include "awaiter.h"
#include "coroutine.h"
galay::coroutine::CommonAwaiter::CommonAwaiter(bool IsSuspend,const std::any& result)
{
    m_IsSuspend = IsSuspend;
    m_Result = result;
}

galay::coroutine::CommonAwaiter::CommonAwaiter(CommonAwaiter&& other)
{
    if(this != &other){
        m_IsSuspend = other.m_IsSuspend;
        m_Result = std::move(other.m_Result);
        other.m_Result.reset();
    }
}

galay::coroutine::CommonAwaiter&
galay::coroutine::CommonAwaiter::operator=(CommonAwaiter&& other)
{
    if(this != &other){
        m_IsSuspend = other.m_IsSuspend;
        m_Result = std::move(other.m_Result);
        other.m_Result.reset();
    }
    return *this;
}

bool 
galay::coroutine::CommonAwaiter::await_ready()
{
    return !m_IsSuspend;
}

void 
galay::coroutine::CommonAwaiter::await_suspend(std::coroutine_handle<> handle) 
{
    
}

std::any 
galay::coroutine::CommonAwaiter::await_resume()
{
    return m_Result;
}

galay::coroutine::GroupAwaiter::GroupAwaiter(bool IsSuspend)
{
    this->m_IsSuspend = IsSuspend;
}

galay::coroutine::GroupAwaiter&
galay::coroutine::GroupAwaiter::operator=(const GroupAwaiter& other)
{
    if(this != &other){
        this->m_coId=other.m_coId;
        this->m_IsSuspend.store(other.m_IsSuspend.load());
    }
    return *this;
}

galay::coroutine::GroupAwaiter::GroupAwaiter(GroupAwaiter&& other)
{
    this->m_coId=other.m_coId;
    this->m_IsSuspend.store(other.m_IsSuspend.load());
}

galay::coroutine::GroupAwaiter&
galay::coroutine::GroupAwaiter::operator=(GroupAwaiter&& other)
{
    if(this != &other){
        this->m_coId=other.m_coId;
        this->m_IsSuspend.store(other.m_IsSuspend.load());
    }
    return *this;
}

bool 
galay::coroutine::GroupAwaiter::await_ready()
{
    return !m_IsSuspend.load(); 
}

void 
galay::coroutine::GroupAwaiter::Resume()
{
    if(this->m_IsSuspend) {
        coroutine::GY_NetCoroutinePool::GetInstance()->Resume(this->m_coId);
        this->m_IsSuspend = false;
    }
}

void 
galay::coroutine::GroupAwaiter::await_suspend(std::coroutine_handle<> handle)
{
    this->m_coId = reinterpret_cast<uint64_t>(handle.address());
}

std::any 
galay::coroutine::GroupAwaiter::await_resume()
{
    return {};
}

