#include "builder.h"
#include "router.h"
#include "../common/coroutine.h"
#include "controller.h"

galay::kernel::GY_SSLConfig::GY_SSLConfig()
{
    m_min_ssl_version.store(DEFAULT_SSL_MIN_VERSION);
    m_max_ssl_version.store(DEFAULT_SSL_MAX_VERSION);
    m_ssl_cert_path = DEFAULT_SSL_CERT_PATH;
    m_ssl_key_path = DEFAULT_SSL_KEY_PATH;
}


void 
galay::kernel::GY_SSLConfig::SetSSLVersion(int32_t min_ssl_version, int32_t max_ssl_version)
{
    m_min_ssl_version.store(min_ssl_version);
    m_max_ssl_version.store(max_ssl_version);
}

void 
galay::kernel::GY_SSLConfig::SetCertPath(const std::string& cert_path)
{
    m_ssl_cert_path = cert_path;
}

void 
galay::kernel::GY_SSLConfig::SetKeyPath(const std::string& key_path)
{
    m_ssl_key_path = key_path;
}

int32_t 
galay::kernel::GY_SSLConfig::GetMinSSLVersion() const
{
    return m_min_ssl_version.load();
}

int32_t 
galay::kernel::GY_SSLConfig::GetMaxSSLVersion() const
{
    return m_max_ssl_version.load();
}


std::string 
galay::kernel::GY_SSLConfig::GetCertPath() const
{
    return m_ssl_cert_path;
}

std::string 
galay::kernel::GY_SSLConfig::GetKeyPath() const 
{
    return m_ssl_key_path;
}

galay::kernel::GY_TcpServerBuilder::GY_TcpServerBuilder()
{
    m_threadnum.store(1);
    m_max_event_size.store(DEFAULT_EVENT_SIZE);
    m_sche_wait_time.store(DEFAULT_SCHE_WAIT_TIME);
    m_rbuffer_len.store(DEFAULT_RBUFFER_LENGTH);
    m_backlog.store(DEFAULT_BACKLOG);
    m_scheduler_type.store(galay::common::SchedulerType::kEpollScheduler);
    m_port = DEFAULT_LISTEN_PORT;
    m_userfunc = nullptr;
    m_is_ssl = false;
    m_ssl_config = nullptr;
    
}

void 
galay::kernel::GY_TcpServerBuilder::SetUserFunction(std::pair<uint16_t,std::function<galay::coroutine::GY_NetCoroutine(galay::kernel::GY_Controller::ptr)>> port_func) 
{
    this->m_port.store(port_func.first);
    this->m_userfunc = port_func.second;
}


void 
galay::kernel::GY_TcpServerBuilder::SetIllegalFunction(std::function<std::string()> func)
{
    this->m_illegalfunc = func;
}


void
galay::kernel::GY_TcpServerBuilder::SetSchedulerType(galay::common::SchedulerType scheduler_type)
{
    this->m_scheduler_type.store(scheduler_type);
}


void
galay::kernel::GY_TcpServerBuilder::SetNetThreadNum(uint16_t threadnum)
{
    m_threadnum.store(threadnum);
}


void
galay::kernel::GY_TcpServerBuilder::SetBacklog(uint16_t backlog)
{
    m_backlog.store(backlog);
}


void 
galay::kernel::GY_TcpServerBuilder::SetPort(uint16_t port)
{
    this->m_port = port;
}


void 
galay::kernel::GY_TcpServerBuilder::SetMaxEventSize(uint16_t max_event_size)
{
    m_max_event_size.store(max_event_size);
}


void 
galay::kernel::GY_TcpServerBuilder::SetScheWaitTime(uint16_t sche_wait_time)
{
    m_sche_wait_time.store(sche_wait_time);
}



void 
galay::kernel::GY_TcpServerBuilder::InitSSLServer(bool is_ssl)
{
    this->m_is_ssl.store(is_ssl);
    m_ssl_config = std::make_shared<galay::kernel::GY_SSLConfig>();
}


uint16_t
galay::kernel::GY_TcpServerBuilder::GetMaxEventSize()
{
    return m_max_event_size.load();
}  


uint16_t
galay::kernel::GY_TcpServerBuilder::GetScheWaitTime()
{
    return m_sche_wait_time.load();
}


uint16_t
galay::kernel::GY_TcpServerBuilder::GetNetThreadNum()
{
    return m_threadnum.load();
}


uint16_t 
galay::kernel::GY_TcpServerBuilder::GetBacklog()
{
    return m_backlog.load();
}


galay::common::SchedulerType 
galay::kernel::GY_TcpServerBuilder::GetSchedulerType()
{
    return m_scheduler_type.load();
}


uint16_t 
galay::kernel::GY_TcpServerBuilder::GetPort()
{
    return m_port.load();
}


std::function<void(galay::kernel::GY_Controller::ptr)>
galay::kernel::GY_TcpServerBuilder::GetUserFunction()
{
    return [this](galay::kernel::GY_Controller::ptr ctrl){
        m_userfunc(ctrl);
    };
}


std::function<std::string()> 
galay::kernel::GY_TcpServerBuilder::GetIllegalFunction()
{
    return this->m_illegalfunc;
}


bool 
galay::kernel::GY_TcpServerBuilder::GetIsSSL()
{
    return m_is_ssl.load();
}


const galay::kernel::GY_SSLConfig::ptr 
galay::kernel::GY_TcpServerBuilder::GetSSLConfig()
{
    return m_ssl_config;
}


std::string 
galay::kernel::GY_TcpServerBuilder::GetTypeName(galay::common::ClassNameType type)
{
    return m_typeNames[type];
}


galay::kernel::GY_TcpServerBuilder::~GY_TcpServerBuilder()
{
    m_ssl_config = nullptr;
}

galay::kernel::GY_HttpServerBuilder::GY_HttpServerBuilder()
{
    m_typeNames[common::kClassNameRequest] = util::GetTypeName<protocol::http::HttpRequest>() ;
    m_typeNames[common::kClassNameResponse] = util::GetTypeName<protocol::http::HttpResponse>() ;
    if(m_typeNames[common::kClassNameRequest].empty() || m_typeNames[common::kClassNameResponse].empty()) {
        spdlog::error("[{}:{}] [protocol type name is empty]", __FILE__, __LINE__);
    }
}

std::function<void(galay::kernel::GY_Controller::ptr)>
galay::kernel::GY_HttpServerBuilder::GetUserFunction()
{
    return [this](galay::kernel::GY_Controller::ptr ctrl){
        this->m_router->RouteHttp(ctrl);
    };
}

void 
galay::kernel::GY_HttpServerBuilder::SetRouter(std::shared_ptr<GY_HttpRouter> router)
{
    this->m_router = router;
}


void 
galay::kernel::GY_HttpServerBuilder::SetUserFunction(std::pair<uint16_t, std::function<coroutine::GY_NetCoroutine(GY_Controller::ptr)>> port_func)
{
    GY_TcpServerBuilder::SetUserFunction(port_func);
}

