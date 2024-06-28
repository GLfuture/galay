#include "awaiter.h"
#include <iostream>
galay::CommonAwaiter::CommonAwaiter(bool IsSuspend,const std::any& result)
{
    m_IsSuspend = IsSuspend;
    m_Result = result;
}

galay::CommonAwaiter::CommonAwaiter(CommonAwaiter&& other)
{
    if(this != &other){
        m_IsSuspend = other.m_IsSuspend;
        m_Result = std::move(other.m_Result);
    }
}

galay::CommonAwaiter&
galay::CommonAwaiter::operator=(CommonAwaiter && other)
{
    if(this != &other){
        m_IsSuspend = other.m_IsSuspend;
        m_Result = std::move(other.m_Result);
    }
    return *this;
}

bool 
galay::CommonAwaiter::await_ready()
{
    return !m_IsSuspend;
}

void 
galay::CommonAwaiter::await_suspend(std::coroutine_handle<> handle) 
{
    
}

std::any 
galay::CommonAwaiter::await_resume()
{
    return m_Result;
}

galay::GroupAwaiter::GroupAwaiter(bool IsSuspend)
{
    this->m_IsSuspend = IsSuspend;
}

galay::GroupAwaiter&
galay::GroupAwaiter::operator=(const GroupAwaiter& other)
{
    if(this != &other){
        this->m_handle=other.m_handle;
        this->m_IsSuspend=other.m_IsSuspend;
    }
    return *this;
}

galay::GroupAwaiter::GroupAwaiter(GroupAwaiter&& other)
{
    this->m_handle=other.m_handle;
    this->m_IsSuspend = other.m_IsSuspend;
}

galay::GroupAwaiter&
galay::GroupAwaiter::operator=(GroupAwaiter&& other)
{
    if(this != &other){
        this->m_handle=other.m_handle;
        this->m_IsSuspend = other.m_IsSuspend;
    }
    return *this;
}

bool 
galay::GroupAwaiter::await_ready()
{
    return !m_IsSuspend;
}

void 
galay::GroupAwaiter::Resume()
{
    if(this->m_handle) 
        this->m_handle.resume();
}

void 
galay::GroupAwaiter::await_suspend(std::coroutine_handle<> handle)
{
    this->m_handle = handle;
}

std::any 
galay::GroupAwaiter::await_resume()
{
    return {};
}


//To Do
galay::HttpAwaiter::HttpAwaiter(bool IsSuspend,std::function<galay::protocol::http::Http1_1_Response::ptr()>& Func,std::queue<std::future<void>>& futures)
    :m_Func(Func),
     m_futures(futures)
{
    this->m_IsSuspend = IsSuspend;
}

galay::HttpAwaiter::HttpAwaiter(HttpAwaiter&& other)
    :m_Func(other.m_Func),
     m_futures(other.m_futures)
{
    if(this != &other){
        this->m_IsSuspend = other.m_IsSuspend;
        this->m_Result = std::move(other.m_Result);
    }
} 
        
galay::HttpAwaiter&
galay::HttpAwaiter::operator=(HttpAwaiter &&other)
{
    if(this != &other){
        this->m_IsSuspend = other.m_IsSuspend;
        this->m_Result = std::move(other.m_Result);
        this->m_Func = other.m_Func;
    }
    return *this;
}

bool 
galay::HttpAwaiter::await_ready()
{
    return !m_IsSuspend;
}

//To Do
void galay::HttpAwaiter::await_suspend(std::coroutine_handle<> handle)
{
    m_futures.emplace(std::async(std::launch::async,[this,handle](){
        m_Result = m_Func();
        handle.resume();
    }));
}

galay::protocol::http::Http1_1_Response::ptr 
galay::HttpAwaiter::await_resume()
{
    return m_Result;
}

galay::SmtpAwaiter::SmtpAwaiter(bool IsSuspend,std::function<std::vector<protocol::smtp::Smtp_Response::ptr>()>& func,std::queue<std::future<void>>& futures)
    :m_Func(func),
     m_futures(futures)
{
    this->m_IsSuspend = IsSuspend;
}

galay::SmtpAwaiter::SmtpAwaiter(SmtpAwaiter&& other)
    :m_Func(other.m_Func),
     m_futures(other.m_futures)
{
    m_IsSuspend = other.m_IsSuspend;
    m_Result = std::move(other.m_Result);
}

galay::SmtpAwaiter& 
galay::SmtpAwaiter::operator=(SmtpAwaiter && other)
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
galay::SmtpAwaiter::await_ready()
{
    return !m_IsSuspend;
}

void 
galay::SmtpAwaiter::await_suspend(std::coroutine_handle<> handle)
{
    this->m_futures.emplace(std::async(std::launch::async,[this,handle](){
        m_Result = m_Func();
        handle.resume();
    }));
}


std::vector<galay::protocol::smtp::Smtp_Response::ptr> 
galay::SmtpAwaiter::await_resume()
{
    return std::move(m_Result);
}

galay::DnsAwaiter::DnsAwaiter(bool IsSuspend,std::function<galay::protocol::dns::Dns_Response::ptr()>& func,std::queue<std::future<void>>& futures)
    :m_Func(func),
     m_futures(futures)
{
    this->m_IsSuspend = IsSuspend;
}

galay::DnsAwaiter::DnsAwaiter(DnsAwaiter&& other)
    :m_Func(other.m_Func),
     m_futures(other.m_futures)
{
    m_IsSuspend = other.m_IsSuspend;
    m_Result = std::move(other.m_Result);
}


galay::DnsAwaiter&
galay::DnsAwaiter::operator=(DnsAwaiter&& other)
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
galay::DnsAwaiter::await_ready()
{
    return !m_IsSuspend;
}

void 
galay::DnsAwaiter::await_suspend(std::coroutine_handle<> handle)
{
    this->m_futures.emplace(std::async(std::launch::async,[this,handle](){
        m_Result = m_Func();
        handle.resume();
    }));
}


galay::protocol::dns::Dns_Response::ptr 
galay::DnsAwaiter::await_resume()
{
    return this->m_Result;
}
