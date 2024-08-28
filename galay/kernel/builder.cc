#include "builder.h"
#include "router.h"
#include "controller.h"
#include "poller.h"
#include "../common/coroutine.h"

namespace galay::server
{

TcpServerBuilder::TcpServerBuilder()
{
    m_threadnum.store(1);
    m_max_event_size.store(DEFAULT_EVENT_SIZE);
    m_sche_wait_time.store(DEFAULT_SCHE_WAIT_TIME);
    m_rbuffer_len.store(DEFAULT_RBUFFER_LENGTH);
    m_backlog.store(DEFAULT_BACKLOG);
    m_pollerType.store(PollerType::kEpollPoller);
    m_port = DEFAULT_LISTEN_PORT;
    m_ssl_config = nullptr;
    
}

void 
TcpServerBuilder::SetRightHandle(std::function<galay::coroutine::NetCoroutine(Controller::ptr)> handle) 
{
    poller::SetRightHandle(handle);
}


void 
TcpServerBuilder::SetWrongHandle(std::function<coroutine::NetCoroutine(Controller::ptr)> handle)
{
    poller::SetWrongHandle(handle);
}


void
TcpServerBuilder::SetPollerType(PollerType scheduler_type)
{
    this->m_pollerType.store(scheduler_type);
}


void
TcpServerBuilder::SetNetThreadNum(uint16_t threadnum)
{
    m_threadnum.store(threadnum);
}


void
TcpServerBuilder::SetBacklog(uint16_t backlog)
{
    m_backlog.store(backlog);
}


void 
TcpServerBuilder::SetPort(uint16_t port)
{
    this->m_port = port;
}


void 
TcpServerBuilder::SetMaxEventSize(uint16_t max_event_size)
{
    m_max_event_size.store(max_event_size);
}


void 
TcpServerBuilder::SetScheWaitTime(uint16_t sche_wait_time)
{
    m_sche_wait_time.store(sche_wait_time);
}



void 
TcpServerBuilder::InitSSLServer(io::net::SSLConfig::ptr config)
{
    m_ssl_config = std::make_shared<io::net::SSLConfig>();
}


uint16_t
TcpServerBuilder::GetMaxEventSize()
{
    return m_max_event_size.load();
}  


uint16_t
TcpServerBuilder::GetScheWaitTime()
{
    return m_sche_wait_time.load();
}


uint16_t
TcpServerBuilder::GetNetThreadNum()
{
    return m_threadnum.load();
}


uint16_t 
TcpServerBuilder::GetBacklog()
{
    return m_backlog.load();
}


PollerType 
TcpServerBuilder::GetSchedulerType()
{
    return m_pollerType.load();
}


uint16_t 
TcpServerBuilder::GetPort()
{
    return m_port.load();
}


bool 
TcpServerBuilder::GetIsSSL()
{
    return m_ssl_config != nullptr;
}


const io::net::SSLConfig::ptr 
TcpServerBuilder::GetSSLConfig()
{
    return m_ssl_config;
}


std::string 
TcpServerBuilder::GetTypeName(ClassNameType type)
{
    return m_typeNames[type];
}


TcpServerBuilder::~TcpServerBuilder()
{
    m_ssl_config = nullptr;
}

HttpServerBuilder::HttpServerBuilder()
{
    m_typeNames[kClassNameRequest] = util::GetTypeName<protocol::http::HttpRequest>() ;
    m_typeNames[kClassNameResponse] = util::GetTypeName<protocol::http::HttpResponse>() ;
    if(m_typeNames[kClassNameRequest].empty() || m_typeNames[kClassNameResponse].empty()) {
        spdlog::error("[{}:{}] [protocol type name is empty]", __FILE__, __LINE__);
    }
}

void 
HttpServerBuilder::SetRouter(std::shared_ptr<HttpRouter> router)
{
    this->m_router = router;
}


void 
HttpServerBuilder::SetRightHandle(std::function<coroutine::NetCoroutine(Controller::ptr)> handle)
{
    TcpServerBuilder::SetRightHandle(handle);
}


void 
HttpServerBuilder::SetWrongHandle(std::function<coroutine::NetCoroutine(Controller::ptr)> handle)
{
    TcpServerBuilder::SetWrongHandle(handle);
}


}
