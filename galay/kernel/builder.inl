#ifndef GALAY_KERNEL_BUILDER_INL_
#define GALAY_KERNEL_BUILDER_INL_

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


template<galay::common::TcpRequest REQ ,galay::common::TcpResponse RESP>
galay::kernel::GY_TcpServerBuilder<REQ,RESP>::GY_TcpServerBuilder()
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
    m_typeNames[common::kClassNameRequest] = util::GetTypeName<REQ>() ;
    m_typeNames[common::kClassNameResponse] = util::GetTypeName<RESP>() ;
    if(m_typeNames[common::kClassNameRequest].empty() || m_typeNames[common::kClassNameResponse].empty()) {
        spdlog::error("[{}:{}] [protocol type name is empty]", __FILE__, __LINE__);
        exit(-1);
    }
}


/// @brief 
/// @tparam REQ 
/// @tparam RESP 
/// @param port_func 
template<galay::common::TcpRequest REQ,galay::common::TcpResponse RESP>
void 
galay::kernel::GY_TcpServerBuilder<REQ,RESP>::SetUserFunction(std::pair<uint16_t,std::function<galay::common::GY_TcpCoroutine<galay::common::CoroutineStatus>(galay::kernel::GY_Controller::wptr)>> port_func) 
{
    this->m_port.store(port_func.first);
    this->m_userfunc = port_func.second;
}

template<galay::common::TcpRequest REQ,galay::common::TcpResponse RESP>
void
galay::kernel::GY_TcpServerBuilder<REQ,RESP>::SetSchedulerType(galay::common::SchedulerType scheduler_type)
{
    this->m_scheduler_type.store(scheduler_type);
}

template<galay::common::TcpRequest REQ,galay::common::TcpResponse RESP>
void
galay::kernel::GY_TcpServerBuilder<REQ,RESP>::SetThreadNum(uint16_t threadnum)
{
    m_threadnum.store(threadnum);
}

template<galay::common::TcpRequest REQ,galay::common::TcpResponse RESP>
void
galay::kernel::GY_TcpServerBuilder<REQ,RESP>::SetBacklog(uint16_t backlog)
{
    m_backlog.store(backlog);
}

template<galay::common::TcpRequest REQ,galay::common::TcpResponse RESP>
void 
galay::kernel::GY_TcpServerBuilder<REQ,RESP>::SetPort(uint16_t port)
{
    this->m_port = port;
}

template<galay::common::TcpRequest REQ,galay::common::TcpResponse RESP>
void 
galay::kernel::GY_TcpServerBuilder<REQ,RESP>::SetMaxEventSize(uint16_t max_event_size)
{
    m_max_event_size.store(max_event_size);
}

template<galay::common::TcpRequest REQ,galay::common::TcpResponse RESP>
void 
galay::kernel::GY_TcpServerBuilder<REQ,RESP>::SetScheWaitTime(uint16_t sche_wait_time)
{
    m_sche_wait_time.store(sche_wait_time);
}

template<galay::common::TcpRequest REQ,galay::common::TcpResponse RESP>
void 
galay::kernel::GY_TcpServerBuilder<REQ,RESP>::SetReadBufferLen(uint32_t rbuffer_len)
{
    m_rbuffer_len.store(rbuffer_len);
}

template<galay::common::TcpRequest REQ,galay::common::TcpResponse RESP>
void 
galay::kernel::GY_TcpServerBuilder<REQ,RESP>::InitSSLServer(bool is_ssl)
{
    this->m_is_ssl.store(is_ssl);
    m_ssl_config = std::make_shared<galay::kernel::GY_SSLConfig>();
}

template<galay::common::TcpRequest REQ,galay::common::TcpResponse RESP>
uint16_t
galay::kernel::GY_TcpServerBuilder<REQ,RESP>::GetMaxEventSize()
{
    return m_max_event_size.load();
}  

template<galay::common::TcpRequest REQ,galay::common::TcpResponse RESP>
uint16_t
galay::kernel::GY_TcpServerBuilder<REQ,RESP>::GetScheWaitTime()
{
    return m_sche_wait_time.load();
}

template<galay::common::TcpRequest REQ,galay::common::TcpResponse RESP>
uint32_t
galay::kernel::GY_TcpServerBuilder<REQ,RESP>::GetReadBufferLen()
{
    return m_rbuffer_len.load();
}

template<galay::common::TcpRequest REQ,galay::common::TcpResponse RESP>
uint16_t
galay::kernel::GY_TcpServerBuilder<REQ,RESP>::GetThreadNum()
{
    return m_threadnum.load();
}

template<galay::common::TcpRequest REQ,galay::common::TcpResponse RESP>
uint16_t 
galay::kernel::GY_TcpServerBuilder<REQ,RESP>::GetBacklog()
{
    return m_backlog.load();
}

template<galay::common::TcpRequest REQ,galay::common::TcpResponse RESP>
galay::common::SchedulerType 
galay::kernel::GY_TcpServerBuilder<REQ,RESP>::GetSchedulerType()
{
    return m_scheduler_type.load();
}

template<galay::common::TcpRequest REQ,galay::common::TcpResponse RESP>
uint16_t 
galay::kernel::GY_TcpServerBuilder<REQ,RESP>::GetPort()
{
    return m_port.load();
}

template<galay::common::TcpRequest REQ,galay::common::TcpResponse RESP>
std::function<galay::common::GY_TcpCoroutine<galay::common::CoroutineStatus>(galay::kernel::GY_Controller::wptr)>
galay::kernel::GY_TcpServerBuilder<REQ,RESP>::GetUserFunction()
{
    return m_userfunc;
}

template<galay::common::TcpRequest REQ,galay::common::TcpResponse RESP>
bool 
galay::kernel::GY_TcpServerBuilder<REQ,RESP>::GetIsSSL()
{
    return m_is_ssl.load();
}

template<galay::common::TcpRequest REQ,galay::common::TcpResponse RESP>
const galay::kernel::GY_SSLConfig::ptr 
galay::kernel::GY_TcpServerBuilder<REQ,RESP>::GetSSLConfig()
{
    return m_ssl_config;
}

template<galay::common::TcpRequest REQ,galay::common::TcpResponse RESP>
std::string 
galay::kernel::GY_TcpServerBuilder<REQ,RESP>::GetTypeName(galay::common::ClassNameType type)
{
    return m_typeNames[type];
}

template<galay::common::TcpRequest REQ,galay::common::TcpResponse RESP>
galay::kernel::GY_TcpServerBuilder<REQ,RESP>::~GY_TcpServerBuilder()
{
    m_ssl_config = nullptr;
}

#endif
