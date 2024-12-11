#include "Server.h"
#include "Event.h"
#include "Scheduler.h"
#include "Async.h"
#include "ExternApi.h"
#include "galay/helper/HttpHelper.h"
#include <openssl/err.h>
#include <utility>

namespace galay::server
{

	
TcpServerConfig::TcpServerConfig()
	: m_backlog(DEFAULT_SERVER_BACKLOG), m_coroutineConf(DEFAULT_COROUTINE_SCHEDULER_CONF), \
	  m_netSchedulerConf(DEFAULT_NETWORK_SCHEDULER_CONF), m_timerSchedulerConf(DEFAULT_TIMER_SCHEDULER_CONF)
{
}

TcpSslServerConfig::TcpSslServerConfig()
	:m_cert_file(nullptr), m_key_file(nullptr)
{
}

HttpServerConfig::HttpServerConfig()
	:m_max_header_size(DEFAULT_HTTP_MAX_HEADER_SIZE)
{
}

HttpsServerConfig::HttpsServerConfig()
	:m_max_header_size(DEFAULT_HTTP_MAX_HEADER_SIZE)
{
}

/*
	Keep-alive 需要处理
*/
}