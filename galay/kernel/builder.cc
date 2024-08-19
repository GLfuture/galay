#include "builder.h"
#include "router.h"
#include "../common/coroutine.h"
#include "controller.h"

namespace galay::server
{
GY_SSLConfig::GY_SSLConfig()
{
    m_min_ssl_version.store(DEFAULT_SSL_MIN_VERSION);
    m_max_ssl_version.store(DEFAULT_SSL_MAX_VERSION);
    m_ssl_cert_path = DEFAULT_SSL_CERT_PATH;
    m_ssl_key_path = DEFAULT_SSL_KEY_PATH;
}

void 
GY_SSLConfig::SetSSLVersion(int32_t min_ssl_version, int32_t max_ssl_version)
{
    m_min_ssl_version.store(min_ssl_version);
    m_max_ssl_version.store(max_ssl_version);
}

void 
GY_SSLConfig::SetCertPath(const std::string& cert_path)
{
    m_ssl_cert_path = cert_path;
}

void 
GY_SSLConfig::SetKeyPath(const std::string& key_path)
{
    m_ssl_key_path = key_path;
}

int32_t 
GY_SSLConfig::GetMinSSLVersion() const
{
    return m_min_ssl_version.load();
}

int32_t 
GY_SSLConfig::GetMaxSSLVersion() const
{
    return m_max_ssl_version.load();
}


std::string 
GY_SSLConfig::GetCertPath() const
{
    return m_ssl_cert_path;
}

std::string 
GY_SSLConfig::GetKeyPath() const 
{
    return m_ssl_key_path;
}

GY_TcpServerBuilder::GY_TcpServerBuilder()
{
    m_threadnum.store(1);
    m_max_event_size.store(DEFAULT_EVENT_SIZE);
    m_sche_wait_time.store(DEFAULT_SCHE_WAIT_TIME);
    m_rbuffer_len.store(DEFAULT_RBUFFER_LENGTH);
    m_backlog.store(DEFAULT_BACKLOG);
    m_scheduler_type.store(galay::common::SchedulerType::kEpollScheduler);
    m_port = DEFAULT_LISTEN_PORT;
    m_rightHandle = nullptr;
    m_is_ssl = false;
    m_ssl_config = nullptr;
    
}

void 
GY_TcpServerBuilder::SetRightHandle(std::function<galay::coroutine::GY_NetCoroutine(GY_Controller::ptr)> handle) 
{
    this->m_rightHandle = handle;
}


void 
GY_TcpServerBuilder::SetWrongHandle(std::function<coroutine::GY_NetCoroutine(GY_Controller::ptr)> func)
{
    this->m_wrongHandle = func;
}


void
GY_TcpServerBuilder::SetSchedulerType(galay::common::SchedulerType scheduler_type)
{
    this->m_scheduler_type.store(scheduler_type);
}


void
GY_TcpServerBuilder::SetNetThreadNum(uint16_t threadnum)
{
    m_threadnum.store(threadnum);
}


void
GY_TcpServerBuilder::SetBacklog(uint16_t backlog)
{
    m_backlog.store(backlog);
}


void 
GY_TcpServerBuilder::SetPort(uint16_t port)
{
    this->m_port = port;
}


void 
GY_TcpServerBuilder::SetMaxEventSize(uint16_t max_event_size)
{
    m_max_event_size.store(max_event_size);
}


void 
GY_TcpServerBuilder::SetScheWaitTime(uint16_t sche_wait_time)
{
    m_sche_wait_time.store(sche_wait_time);
}



void 
GY_TcpServerBuilder::InitSSLServer(bool is_ssl)
{
    this->m_is_ssl.store(is_ssl);
    m_ssl_config = std::make_shared<GY_SSLConfig>();
}


uint16_t
GY_TcpServerBuilder::GetMaxEventSize()
{
    return m_max_event_size.load();
}  


uint16_t
GY_TcpServerBuilder::GetScheWaitTime()
{
    return m_sche_wait_time.load();
}


uint16_t
GY_TcpServerBuilder::GetNetThreadNum()
{
    return m_threadnum.load();
}


uint16_t 
GY_TcpServerBuilder::GetBacklog()
{
    return m_backlog.load();
}


galay::common::SchedulerType 
GY_TcpServerBuilder::GetSchedulerType()
{
    return m_scheduler_type.load();
}


uint16_t 
GY_TcpServerBuilder::GetPort()
{
    return m_port.load();
}


std::function<void(GY_Controller::ptr)>
GY_TcpServerBuilder::GetRightHandle()
{
    return [this](GY_Controller::ptr ctrl){
        m_rightHandle(ctrl);
    };
}


std::function<void(GY_Controller::ptr)> 
GY_TcpServerBuilder::GetWrongHandle()
{
    return [this](GY_Controller::ptr ctrl){
        this->m_wrongHandle(ctrl);
    };
}


bool 
GY_TcpServerBuilder::GetIsSSL()
{
    return m_is_ssl.load();
}


const GY_SSLConfig::ptr 
GY_TcpServerBuilder::GetSSLConfig()
{
    return m_ssl_config;
}


std::string 
GY_TcpServerBuilder::GetTypeName(galay::common::ClassNameType type)
{
    return m_typeNames[type];
}


GY_TcpServerBuilder::~GY_TcpServerBuilder()
{
    m_ssl_config = nullptr;
}

GY_HttpServerBuilder::GY_HttpServerBuilder()
{
    m_typeNames[common::kClassNameRequest] = util::GetTypeName<protocol::http::HttpRequest>() ;
    m_typeNames[common::kClassNameResponse] = util::GetTypeName<protocol::http::HttpResponse>() ;
    if(m_typeNames[common::kClassNameRequest].empty() || m_typeNames[common::kClassNameResponse].empty()) {
        spdlog::error("[{}:{}] [protocol type name is empty]", __FILE__, __LINE__);
    }
}

std::function<void(GY_Controller::ptr)>
GY_HttpServerBuilder::GetRightHandle()
{
    return [this](GY_Controller::ptr ctrl){
        this->m_router->RouteRightHttp(ctrl);
    };
}

std::function<void(GY_Controller::ptr)> 
GY_HttpServerBuilder::GetWrongHandle()
{
    return [this](GY_Controller::ptr ctrl){
        this->m_router->RouteWrongHttp(ctrl);
    };
}

void 
GY_HttpServerBuilder::SetRouter(std::shared_ptr<GY_HttpRouter> router)
{
    this->m_router = router;
}


void 
GY_HttpServerBuilder::SetRightHandle(std::function<coroutine::GY_NetCoroutine(GY_Controller::ptr)> handle)
{
    GY_TcpServerBuilder::SetRightHandle(handle);
}


void 
GY_HttpServerBuilder::SetWrongHandle(std::function<coroutine::GY_NetCoroutine(GY_Controller::ptr)> handle)
{
    GY_TcpServerBuilder::SetWrongHandle(handle);
}


}
