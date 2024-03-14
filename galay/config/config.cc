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
galay::Tcp_Server_Config::Tcp_Server_Config(std::vector<uint16_t> ports, uint32_t backlog, uint32_t recv_len,int conn_timeout,Engine_Type type,int sche_wait_time,int max_event_size,int threadnum)
    : m_ports(ports), m_backlog(backlog), m_max_rbuffer_len(recv_len),m_conn_timeout(conn_timeout),Config(type,sche_wait_time,max_event_size,threadnum)
{
}

galay::Tcp_Server_Config::Tcp_Server_Config(Tcp_Server_Config &&other)
    :Config(other)
{
    this->m_backlog = other.m_backlog;
    this->m_ports = other.m_ports;
    this->m_max_rbuffer_len = other.m_max_rbuffer_len;
    this->m_conn_timeout = other.m_conn_timeout;
}

galay::Tcp_Server_Config::Tcp_Server_Config(const Tcp_Server_Config &other)
    :Config(other)
{
    this->m_backlog = other.m_backlog;
    this->m_ports = other.m_ports;
    this->m_max_rbuffer_len = other.m_max_rbuffer_len;
    this->m_conn_timeout = other.m_conn_timeout;
}

void galay::Tcp_Server_Config::enable_keepalive(int t_idle, int t_interval, int retry)
{
    Tcp_Keepalive_Config keepalive{
        .m_keepalive = true,
        .m_idle = t_idle,
        .m_interval = t_interval,
        .m_retry = retry
    };
    this->m_keepalive_conf = keepalive;
}

// tcp ssl server config
galay::Tcp_SSL_Server_Config::Tcp_SSL_Server_Config(std::vector<uint16_t> ports, uint32_t backlog, uint32_t recv_len,int conn_timeout,long ssl_min_version,long ssl_max_version
, uint32_t ssl_accept_max_retry, const char *cert_filepath, const char *key_filepath,Engine_Type type,int sche_wait_time,int max_event_size,int threadnum)
        : Tcp_Server_Config(ports, backlog, recv_len,conn_timeout,type,sche_wait_time,max_event_size,threadnum)
            , m_ssl_min_version(ssl_min_version), m_ssl_max_version(ssl_max_version), m_ssl_accept_retry(ssl_accept_max_retry), m_cert_filepath(cert_filepath), m_key_filepath(key_filepath)
{
}

galay::Tcp_SSL_Server_Config::Tcp_SSL_Server_Config(const Tcp_SSL_Server_Config &other)
    : Tcp_Server_Config(other)
{
    this->m_ssl_min_version = other.m_ssl_min_version;
    this->m_ssl_max_version = other.m_ssl_max_version;
    this->m_cert_filepath = other.m_cert_filepath;
    this->m_key_filepath = other.m_key_filepath;
    this->m_ssl_accept_retry = other.m_ssl_accept_retry;
}

galay::Tcp_SSL_Server_Config::Tcp_SSL_Server_Config(Tcp_SSL_Server_Config &&other)
    : Tcp_Server_Config(std::forward<Tcp_SSL_Server_Config>(other))
{
    this->m_ssl_min_version = other.m_ssl_min_version;
    this->m_ssl_max_version = other.m_ssl_max_version;
    this->m_cert_filepath = std::move(other.m_cert_filepath);
    this->m_key_filepath = std::move(other.m_key_filepath);
    this->m_ssl_accept_retry = other.m_ssl_accept_retry;
}

// http server config
galay::Http_Server_Config::Http_Server_Config(std::vector<uint16_t> ports, uint32_t backlog, uint32_t recv_len,int conn_timeout,Engine_Type type,int sche_wait_time,int max_event_size,int threadnum)
    : Tcp_Server_Config(ports, backlog, recv_len,conn_timeout,type,sche_wait_time,max_event_size,threadnum)
{
}

galay::Http_Server_Config::Http_Server_Config(const Http_Server_Config &other)
    : Tcp_Server_Config(other)
{
}

galay::Http_Server_Config::Http_Server_Config(Http_Server_Config &&other)
    : Tcp_Server_Config(std::forward<Http_Server_Config>(other))
{
}

galay::Https_Server_Config::Https_Server_Config(std::vector<uint16_t> ports, uint32_t backlog, uint32_t recv_len , int conn_timeout,long ssl_min_version,long ssl_max_version
    ,uint32_t ssl_accept_max_retry, const char *cert_filepath, const char *key_filepath,Engine_Type type,int sche_wait_time,int max_event_size,int threadnum)
    : Tcp_SSL_Server_Config(ports, backlog, recv_len , conn_timeout, ssl_min_version, ssl_max_version, ssl_accept_max_retry
        , cert_filepath, key_filepath,type,sche_wait_time,max_event_size,threadnum)
{
}

galay::Https_Server_Config::Https_Server_Config(const Https_Server_Config &other)
    : Tcp_SSL_Server_Config(other)
{
}

galay::Https_Server_Config::Https_Server_Config(Https_Server_Config &&other)
    : Tcp_SSL_Server_Config(std::forward<Https_Server_Config>(other))
{
}

//udp server

galay::Udp_Server_Config::Udp_Server_Config(std::vector<uint16_t> ports,Engine_Type type,int sche_wait_time,int max_event_size,int threadnum)
    :Config(type,sche_wait_time,max_event_size,threadnum)
{
    this->m_ports = ports;
}

galay::Udp_Server_Config::Udp_Server_Config(Udp_Server_Config&& other)
    :Config(other)
{
    this->m_ports = std::move(other.m_ports);
}

galay::Udp_Server_Config::Udp_Server_Config(const Udp_Server_Config& other)
    :Config(other)
{
    this->m_ports = other.m_ports;
}