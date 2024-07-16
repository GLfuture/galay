#include "awaiter.h"
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
    }
}

galay::common::CommonAwaiter&
galay::common::CommonAwaiter::operator=(CommonAwaiter && other)
{
    if(this != &other){
        m_IsSuspend = other.m_IsSuspend;
        m_Result = std::move(other.m_Result);
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

galay::common::GroupAwaiter::GroupAwaiter(std::weak_ptr<common::GY_NetCoroutinePool> coPool, bool IsSuspend)
{
    this->m_IsSuspend = IsSuspend;
    this->m_coPool = coPool;
}

galay::common::GroupAwaiter&
galay::common::GroupAwaiter::operator=(const GroupAwaiter& other)
{
    if(this != &other){
        this->m_coId=other.m_coId;
        this->m_coPool=other.m_coPool;
        this->m_IsSuspend=other.m_IsSuspend;
    }
    return *this;
}

galay::common::GroupAwaiter::GroupAwaiter(GroupAwaiter&& other)
{
    this->m_coId=other.m_coId;
    this->m_coPool=other.m_coPool;
    this->m_IsSuspend = other.m_IsSuspend;
}

galay::common::GroupAwaiter&
galay::common::GroupAwaiter::operator=(GroupAwaiter&& other)
{
    if(this != &other){
        this->m_coId=other.m_coId;
        this->m_coPool=other.m_coPool;
        this->m_IsSuspend = other.m_IsSuspend;
    }
    return *this;
}

bool 
galay::common::GroupAwaiter::await_ready()
{
    return !m_IsSuspend; 
}

void 
galay::common::GroupAwaiter::Resume()
{
    if(this->m_IsSuspend) this->m_coPool.lock()->Resume(this->m_coId,false);
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


//To Do
galay::common::HttpAwaiter::HttpAwaiter(bool IsSuspend,std::function<galay::protocol::http::HttpResponse::ptr()>& Func,std::queue<std::future<void>>& futures)
    :m_Func(Func),
     m_futures(futures)
{
    this->m_IsSuspend = IsSuspend;
}

galay::common::HttpAwaiter::HttpAwaiter(HttpAwaiter&& other)
    :m_Func(other.m_Func),
     m_futures(other.m_futures)
{
    if(this != &other){
        this->m_IsSuspend = other.m_IsSuspend;
        this->m_Result = std::move(other.m_Result);
    }
} 
        
galay::common::HttpAwaiter&
galay::common::HttpAwaiter::operator=(HttpAwaiter &&other)
{
    if(this != &other){
        this->m_IsSuspend = other.m_IsSuspend;
        this->m_Result = std::move(other.m_Result);
        this->m_Func = other.m_Func;
    }
    return *this;
}

bool 
galay::common::HttpAwaiter::await_ready()
{
    return !m_IsSuspend;
}

//To Do
void 
galay::common::HttpAwaiter::await_suspend(std::coroutine_handle<> handle)
{
    m_futures.emplace(std::async(std::launch::async,[this,handle](){
        m_Result = m_Func();
        handle.resume();
    }));
}

galay::protocol::http::HttpResponse::ptr 
galay::common::HttpAwaiter::await_resume()
{
    return m_Result;
}

galay::common::SmtpAwaiter::SmtpAwaiter(bool IsSuspend,std::function<std::vector<protocol::smtp::Smtp_Response::ptr>()>& func,std::queue<std::future<void>>& futures)
    :m_Func(func),
     m_futures(futures)
{
    this->m_IsSuspend = IsSuspend;
}

galay::common::SmtpAwaiter::SmtpAwaiter(SmtpAwaiter&& other)
    :m_Func(other.m_Func),
     m_futures(other.m_futures)
{
    m_IsSuspend = other.m_IsSuspend;
    m_Result = std::move(other.m_Result);
}

galay::common::SmtpAwaiter& 
galay::common::SmtpAwaiter::operator=(SmtpAwaiter && other)
{
    if(this != &other)
    {
        m_Func = other.m_Func;
        m_IsSuspend = other.m_IsSuspend;
        m_Result = std::move(other.m_Result);
    }
    return *this;
}

bool 
galay::common::SmtpAwaiter::await_ready()
{
    return !m_IsSuspend;
}

void 
galay::common::SmtpAwaiter::await_suspend(std::coroutine_handle<> handle)
{
    this->m_futures.emplace(std::async(std::launch::async,[this,handle](){
        m_Result = m_Func();
        handle.resume();
    }));
}


std::vector<galay::protocol::smtp::Smtp_Response::ptr> 
galay::common::SmtpAwaiter::await_resume()
{
    return std::move(m_Result);
}

galay::common::DnsAwaiter::DnsAwaiter(bool IsSuspend,std::function<galay::protocol::dns::Dns_Response::ptr()>& func,std::queue<std::future<void>>& futures)
    :m_Func(func),
     m_futures(futures)
{
    this->m_IsSuspend = IsSuspend;
}

galay::common::DnsAwaiter::DnsAwaiter(DnsAwaiter&& other)
    :m_Func(other.m_Func),
     m_futures(other.m_futures)
{
    m_IsSuspend = other.m_IsSuspend;
    m_Result = std::move(other.m_Result);
}


galay::common::DnsAwaiter&
galay::common::DnsAwaiter::operator=(DnsAwaiter&& other)
{
    if(this != &other)
    {
        this->m_Func = other.m_Func;
        this->m_IsSuspend = other.m_IsSuspend;
        this->m_Result = std::move(other.m_Result);
    }
    return *this;
}

bool 
galay::common::DnsAwaiter::await_ready()
{
    return !m_IsSuspend;
}

void 
galay::common::DnsAwaiter::await_suspend(std::coroutine_handle<> handle)
{
    this->m_futures.emplace(std::async(std::launch::async,[this,handle](){
        m_Result = m_Func();
        handle.resume();
    }));
}


galay::protocol::dns::Dns_Response::ptr 
galay::common::DnsAwaiter::await_resume()
{
    return this->m_Result;
}
