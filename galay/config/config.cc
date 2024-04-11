#include "config.h"

galay::Config::Config(Engine_Type type,int sche_wait_time,int max_event_size,int threadnum)
{
    this->m_threadnum = threadnum;
    this->m_type = type;
    this->m_sche_wait_time = sche_wait_time;
    this->m_max_event_size = max_event_size;
}

galay::Config::Config(Config &&other)
{
    this->m_threadnum = other.m_threadnum;
    this->m_type = other.m_type;
    this->m_sche_wait_time = other.m_sche_wait_time;
    this->m_max_event_size = other.m_max_event_size;
}

galay::Config::Config(const Config &other)
{
    this->m_threadnum = other.m_threadnum;
    this->m_type = other.m_type;
    this->m_sche_wait_time = other.m_sche_wait_time;
    this->m_max_event_size = other.m_max_event_size;
}

// tcp server config
galay::TcpServerConf::TcpServerConf(uint32_t backlog, uint32_t recv_len,Engine_Type type,int sche_wait_time,int max_event_size,int threadnum)
    : m_backlog(backlog), m_max_rbuffer_len(recv_len),Config(type,sche_wait_time,max_event_size,threadnum)
{
}

galay::TcpServerConf::TcpServerConf(TcpServerConf &&other)
    :Config(other)
{
    this->m_backlog = other.m_backlog;
    this->m_max_rbuffer_len = other.m_max_rbuffer_len;
    this->m_conn_timeout = other.m_conn_timeout;
}

galay::TcpServerConf::TcpServerConf(const TcpServerConf &other)
    :Config(other)
{
    this->m_backlog = other.m_backlog;
    this->m_max_rbuffer_len = other.m_max_rbuffer_len;
    this->m_conn_timeout = other.m_conn_timeout;
}

void galay::TcpServerConf::SetKeepalive(int t_idle, int t_interval, int retry)
{
    TcpKeepaliveConf keepalive{
        .m_keepalive = true,
        .m_idle = t_idle,
        .m_interval = t_interval,
        .m_retry = retry
    };
    this->m_keepalive_conf = keepalive;
}

void galay::TcpServerConf::SetConnTimeout(int t_timeout)
{
    this->m_conn_timeout.m_timeout = t_timeout;
}

// tcp ssl server config
galay::TcpSSLServerConf::TcpSSLServerConf( uint32_t backlog, uint32_t recv_len,Engine_Type type,int sche_wait_time,int max_event_size,int threadnum)
        : TcpServerConf(backlog, recv_len,type,sche_wait_time,max_event_size,threadnum)
{
}

galay::TcpSSLServerConf::TcpSSLServerConf(const TcpSSLServerConf &other)
    : TcpServerConf(other)
{
   this->m_ssl_conf = other.m_ssl_conf;
}

galay::TcpSSLServerConf::TcpSSLServerConf(TcpSSLServerConf &&other)
    : TcpServerConf(std::forward<TcpSSLServerConf>(other))
{
    this->m_ssl_conf = other.m_ssl_conf;
}

void galay::TcpSSLServerConf::SetSSLConf(long min_version,long max_version,uint32_t max_accept_retry,uint32_t sleep_misc_per_retry,const char* cert_filepath,const char* key_filepath)
{
    this->m_ssl_conf.m_min_version = min_version;
    this->m_ssl_conf.m_max_version = max_version;
    this->m_ssl_conf.m_max_accept_retry = max_accept_retry;
    this->m_ssl_conf.m_sleep_misc_per_retry = sleep_misc_per_retry;
    this->m_ssl_conf.m_cert_filepath = cert_filepath;
    this->m_ssl_conf.m_key_filepath = key_filepath;
}

// http server config
galay::HttpServerConf::HttpServerConf(uint32_t backlog, uint32_t recv_len,Engine_Type type,int sche_wait_time,int max_event_size,int threadnum)
    : TcpServerConf(backlog, recv_len,type,sche_wait_time,max_event_size,threadnum)
{
}

galay::HttpServerConf::HttpServerConf(const HttpServerConf &other)
    : TcpServerConf(other)
{
}

galay::HttpServerConf::HttpServerConf(HttpServerConf &&other)
    : TcpServerConf(std::forward<HttpServerConf>(other))
{
}

galay::HttpSSLServerConf::HttpSSLServerConf(uint32_t backlog, uint32_t recv_len,Engine_Type type,int sche_wait_time,int max_event_size,int threadnum)
    : TcpSSLServerConf(backlog, recv_len,type,sche_wait_time,max_event_size,threadnum)
{
}

galay::HttpSSLServerConf::HttpSSLServerConf(const HttpSSLServerConf &other)
    : TcpSSLServerConf(other)
{
}

galay::HttpSSLServerConf::HttpSSLServerConf(HttpSSLServerConf &&other)
    : TcpSSLServerConf(std::forward<HttpSSLServerConf>(other))
{
}

//udp server

galay::UdpServerConf::UdpServerConf(Engine_Type type,int sche_wait_time,int max_event_size,int threadnum)
    :Config(type,sche_wait_time,max_event_size,threadnum)
{
}

galay::UdpServerConf::UdpServerConf(UdpServerConf&& other)
    :Config(other)
{
}

galay::UdpServerConf::UdpServerConf(const UdpServerConf& other)
    :Config(other)
{
}