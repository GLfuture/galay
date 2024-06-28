#ifndef GALAY_KERNEL_BUILDER_INL_
#define GALAY_KERNEL_BUILDER_INL_

galay::GY_SSLConfig::GY_SSLConfig()
{
    m_min_ssl_version.store(DEFAULT_SSL_MIN_VERSION);
    m_max_ssl_version.store(DEFAULT_SSL_MAX_VERSION);
    m_ssl_cert_path = DEFAULT_SSL_CERT_PATH;
    m_ssl_key_path = DEFAULT_SSL_KEY_PATH;
}


void 
galay::GY_SSLConfig::SetSSLVersion(int32_t min_ssl_version, int32_t max_ssl_version)
{
    m_min_ssl_version.store(min_ssl_version);
    m_max_ssl_version.store(max_ssl_version);
}

void 
galay::GY_SSLConfig::SetCertPath(const std::string& cert_path)
{
    m_ssl_cert_path = cert_path;
}

void 
galay::GY_SSLConfig::SetKeyPath(const std::string& key_path)
{
    m_ssl_key_path = key_path;
}

int32_t 
galay::GY_SSLConfig::GetMinSSLVersion() const
{
    return m_min_ssl_version.load();
}

int32_t 
galay::GY_SSLConfig::GetMaxSSLVersion() const
{
    return m_max_ssl_version.load();
}


std::string 
galay::GY_SSLConfig::GetCertPath() const
{
    return m_ssl_cert_path;
}

std::string 
galay::GY_SSLConfig::GetKeyPath() const 
{
    return m_ssl_key_path;
}


template<galay::Tcp_Request REQ,galay::Tcp_Response RESP>
galay::GY_TcpServerBuilder<REQ,RESP>::GY_TcpServerBuilder()
{
    m_threadnum.store(1);
    m_max_event_size.store(DEFAULT_EVENT_SIZE);
    m_sche_wait_time.store(DEFAULT_SCHE_WAIT_TIME);
    m_rbuffer_len.store(DEFAULT_RBUFFER_LENGTH);
    m_backlog.store(DEFAULT_BACKLOG);
    m_scheduler_type.store(SchedulerType::kEpollScheduler);
    m_port = DEFAULT_LISTEN_PORT;
    m_userfunc = nullptr;
    m_is_ssl = false;
    m_ssl_config = nullptr;
    m_typeNames[kDataRequest] = REQ::GetTypeName();
    m_typeNames[kDataResponse] = RESP::GetTypeName();
}

template<galay::Tcp_Request REQ,galay::Tcp_Response RESP>
void 
galay::GY_TcpServerBuilder<REQ,RESP>::SetIllegalFunction(std::function<GY_TcpCoroutine<galay::CoroutineStatus>(std::string&,std::string&)> func)
{
    m_illegalfunc = func;
}

/// @brief 
/// @tparam REQ 
/// @tparam RESP 
/// @param port_func 
template<galay::Tcp_Request REQ,galay::Tcp_Response RESP>
void 
galay::GY_TcpServerBuilder<REQ,RESP>::SetUserFunction(std::pair<uint16_t,std::function<GY_TcpCoroutine<galay::CoroutineStatus>(GY_Controller::wptr)>> port_func) 
{
    this->m_port.store(port_func.first);
    this->m_userfunc = port_func.second;
}

template<galay::Tcp_Request REQ,galay::Tcp_Response RESP>
void
galay::GY_TcpServerBuilder<REQ,RESP>::SetSchedulerType(SchedulerType scheduler_type)
{
    this->m_scheduler_type.store(scheduler_type);
}

template<galay::Tcp_Request REQ,galay::Tcp_Response RESP>
void
galay::GY_TcpServerBuilder<REQ,RESP>::SetThreadNum(uint16_t threadnum)
{
    m_threadnum.store(threadnum);
}

template<galay::Tcp_Request REQ,galay::Tcp_Response RESP>
void
galay::GY_TcpServerBuilder<REQ,RESP>::SetBacklog(uint16_t backlog)
{
    m_backlog.store(backlog);
}

template<galay::Tcp_Request REQ,galay::Tcp_Response RESP>
void 
galay::GY_TcpServerBuilder<REQ,RESP>::SetPort(uint16_t port)
{
    this->m_port = port;
}

template<galay::Tcp_Request REQ,galay::Tcp_Response RESP>
void 
galay::GY_TcpServerBuilder<REQ,RESP>::SetMaxEventSize(uint16_t max_event_size)
{
    m_max_event_size.store(max_event_size);
}

template<galay::Tcp_Request REQ,galay::Tcp_Response RESP>
void 
galay::GY_TcpServerBuilder<REQ,RESP>::SetScheWaitTime(uint16_t sche_wait_time)
{
    m_sche_wait_time.store(sche_wait_time);
}

template<galay::Tcp_Request REQ,galay::Tcp_Response RESP>
void 
galay::GY_TcpServerBuilder<REQ,RESP>::SetReadBufferLen(uint32_t rbuffer_len)
{
    m_rbuffer_len.store(rbuffer_len);
}

template<galay::Tcp_Request REQ,galay::Tcp_Response RESP>
void 
galay::GY_TcpServerBuilder<REQ,RESP>::InitSSLServer(bool is_ssl)
{
    this->m_is_ssl.store(is_ssl);
    m_ssl_config = std::make_shared<GY_SSLConfig>();
}

template<galay::Tcp_Request REQ,galay::Tcp_Response RESP>
uint16_t
galay::GY_TcpServerBuilder<REQ,RESP>::GetMaxEventSize()
{
    return m_max_event_size.load();
}  

template<galay::Tcp_Request REQ,galay::Tcp_Response RESP>
uint16_t
galay::GY_TcpServerBuilder<REQ,RESP>::GetScheWaitTime()
{
    return m_sche_wait_time.load();
}

template<galay::Tcp_Request REQ,galay::Tcp_Response RESP>
uint32_t
galay::GY_TcpServerBuilder<REQ,RESP>::GetReadBufferLen()
{
    return m_rbuffer_len.load();
}

template<galay::Tcp_Request REQ,galay::Tcp_Response RESP>
uint16_t
galay::GY_TcpServerBuilder<REQ,RESP>::GetThreadNum()
{
    return m_threadnum.load();
}

template<galay::Tcp_Request REQ,galay::Tcp_Response RESP>
uint16_t 
galay::GY_TcpServerBuilder<REQ,RESP>::GetBacklog()
{
    return m_backlog.load();
}

template<galay::Tcp_Request REQ,galay::Tcp_Response RESP>
galay::SchedulerType 
galay::GY_TcpServerBuilder<REQ,RESP>::GetSchedulerType()
{
    return m_scheduler_type.load();
}

template<galay::Tcp_Request REQ,galay::Tcp_Response RESP>
uint16_t 
galay::GY_TcpServerBuilder<REQ,RESP>::GetPort()
{
    return m_port.load();
}

template<galay::Tcp_Request REQ,galay::Tcp_Response RESP>
std::function<galay::GY_TcpCoroutine<galay::CoroutineStatus>(galay::GY_Controller::wptr)>
galay::GY_TcpServerBuilder<REQ,RESP>::GetUserFunction()
{
    return m_userfunc;
}

template<galay::Tcp_Request REQ,galay::Tcp_Response RESP>
std::function<galay::GY_TcpCoroutine<galay::CoroutineStatus>(std::string&,std::string&)> 
galay::GY_TcpServerBuilder<REQ,RESP>::GetIllegalFunction()
{
    return m_illegalfunc;
}

template<galay::Tcp_Request REQ,galay::Tcp_Response RESP>
bool 
galay::GY_TcpServerBuilder<REQ,RESP>::GetIsSSL()
{
    return m_is_ssl.load();
}

template<galay::Tcp_Request REQ,galay::Tcp_Response RESP>
const galay::GY_SSLConfig::ptr 
galay::GY_TcpServerBuilder<REQ,RESP>::GetSSLConfig()
{
    return m_ssl_config;
}

template<galay::Tcp_Request REQ,galay::Tcp_Response RESP>
std::string 
galay::GY_TcpServerBuilder<REQ,RESP>::GetTypeName(DataType type)
{
    switch(type)
    {
        case galay::DataType::kDataRequest :
            return m_typeNames[kDataRequest];
        case galay::DataType::kDataResponse :
            return m_typeNames[kDataRequest];
    }
    return "";
}

template<galay::Tcp_Request REQ,galay::Tcp_Response RESP>
galay::GY_TcpServerBuilder<REQ,RESP>::~GY_TcpServerBuilder()
{
    m_ssl_config = nullptr;
}

#endif
