#include "config.h"

// tcp server config
galay::Tcp_Server_Config::Tcp_Server_Config(uint16_t port, uint32_t backlog, uint32_t recv_len)
    : m_port(port), m_backlog(backlog), m_max_rbuffer_len(recv_len)
{
}

galay::Tcp_Server_Config::Tcp_Server_Config(Tcp_Server_Config &&other)
{
    this->m_backlog = other.m_backlog;
    this->m_port = other.m_port;
    this->m_max_rbuffer_len = other.m_max_rbuffer_len;
}

galay::Tcp_Server_Config::Tcp_Server_Config(const Tcp_Server_Config &other)
{
    this->m_backlog = other.m_backlog;
    this->m_port = other.m_port;
    this->m_max_rbuffer_len = other.m_max_rbuffer_len;
}

void galay::Tcp_Server_Config::enable_keepalive(int t_idle, int t_interval, int retry)
{
    this->m_keepalive = true;
    this->m_idle = t_idle;
    this->m_interval = t_interval;
    this->m_retry = retry;
}

// tcp ssl server config
galay::Tcp_SSL_Server_Config::Tcp_SSL_Server_Config(uint16_t port, uint32_t backlog, uint32_t recv_len, long ssl_min_version, long ssl_max_version, uint32_t ssl_accept_max_retry, const char *cert_filepath, const char *key_filepath)
    : Tcp_Server_Config(port, backlog, recv_len), m_ssl_min_version(ssl_min_version), m_ssl_max_version(ssl_max_version), m_ssl_accept_retry(ssl_accept_max_retry), m_cert_filepath(cert_filepath), m_key_filepath(key_filepath)
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
galay::Http_Server_Config::Http_Server_Config(uint16_t port, uint32_t backlog, uint32_t recv_len)
    : Tcp_Server_Config(port, backlog, recv_len)
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

galay::Https_Server_Config::Https_Server_Config(uint16_t port, uint32_t backlog, uint32_t recv_len, long ssl_min_version, long ssl_max_version, uint32_t ssl_accept_max_retry, const char *cert_filepath, const char *key_filepath)
    : Tcp_SSL_Server_Config(port, backlog, recv_len, ssl_min_version, ssl_max_version, ssl_accept_max_retry, cert_filepath, key_filepath)
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

galay::Udp_Server_Config::Udp_Server_Config(uint16_t port)
{
    this->m_port = port;
}

galay::Udp_Server_Config::Udp_Server_Config(Udp_Server_Config&& other)
{
    this->m_port = other.m_port;
}

galay::Udp_Server_Config::Udp_Server_Config(const Udp_Server_Config& other)
{
    this->m_port = other.m_port;
}