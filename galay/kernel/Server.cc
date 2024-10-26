#include "Server.h"
#include "Event.h"
#include "Scheduler.h"
#include "Async.h"
#include "Operation.h"
#include "Step.h"
#include <openssl/err.h>
#include "spdlog/spdlog.h"

namespace galay::server
{
TcpServer::TcpServer()
	: m_co_sche_num(DEFAULT_SERVER_CO_SCHEDULER_NUM), m_net_sche_num(DEFAULT_SERVER_NET_SCHEDULER_NUM), m_co_sche_timeout(-1)\
		, m_net_sche_timeout(-1), m_socket(nullptr), m_is_running(false)
{
}

coroutine::Coroutine TcpServer::Start(TcpCallbackStore* store, int port, int backlog)
{
	
	DynamicResizeCoroutineSchedulers(m_co_sche_num);
	DynamicResizeNetIOSchedulers(m_net_sche_num);
	StartCoroutineSchedulers(m_co_sche_timeout);
	StartNetIOSchedulers(m_net_sche_timeout);
	m_is_running = true;
	
	
	m_socket = new async::AsyncTcpSocket();
	event::TcpWaitEvent event(nullptr, m_socket);
	action::TcpEventAction action(&event);
	bool res = co_await m_socket->Socket(&action);
	m_socket->GetOption().HandleNonBlock();
	m_socket->GetOption().HandleReuseAddr();
	m_socket->GetOption().HandleReusePort();

	m_socket->BindAndListen(port, backlog);
	m_listen_events.resize(m_net_sche_num);
	for(int i = 0 ; i < m_net_sche_num; ++i )
	{
		m_listen_events[i] = new event::ListenEvent(m_socket, store\
			, GetNetIOScheduler(i), GetCoroutineScheduler(i));
		GetNetIOScheduler(i)->AddEvent(m_listen_events[i]);
	} 
	co_return;
}

void TcpServer::Stop()
{
	if(!m_is_running) return;
	for(int i = 0 ; i < m_net_sche_num; ++i )
	{
		m_listen_events[i]->Free(GetNetIOScheduler(i)->GetEngine());
	}
	StopCoroutineSchedulers();
	StopNetIOSchedulers();
	m_is_running = false;
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void TcpServer::ReSetCoroutineSchedulerNum(int num)
{
	if(m_is_running) 
		DynamicResizeCoroutineSchedulers(num);
	else
		m_co_sche_num = num;
}

async::AsyncTcpSocket *TcpServer::GetSocket()
{
    return m_socket;
}

void TcpServer::ReSetNetworkSchedulerNum(int num)
{
	if(m_is_running)
		DynamicResizeNetIOSchedulers(num);
	else 
		m_net_sche_num = num;
}

TcpServer::~TcpServer()
{
	if( m_socket ) {
		delete m_socket;
	}
}

TcpSslServer::TcpSslServer(const char* cert_file, const char* key_file)
	: m_co_sche_num(DEFAULT_SERVER_CO_SCHEDULER_NUM), m_net_sche_num(DEFAULT_SERVER_NET_SCHEDULER_NUM), m_co_sche_timeout(-1)\
		, m_net_sche_timeout(-1), m_socket(nullptr), m_is_running(false), m_cert_file(cert_file), m_key_file(key_file)
{
}

coroutine::Coroutine TcpSslServer::Start(TcpSslCallbackStore *store, int port, int backlog)
{
    DynamicResizeCoroutineSchedulers(m_co_sche_num);
	DynamicResizeNetIOSchedulers(m_net_sche_num);
	StartCoroutineSchedulers(m_co_sche_timeout);
	StartNetIOSchedulers(m_net_sche_timeout);
	
	bool res = InitialSSLServerEnv(m_cert_file, m_key_file);
	if( !res ) {
		spdlog::error("InitialSSLServerEnv failed, error:{}", ERR_error_string(ERR_get_error(), nullptr));
		co_return;
	}
	m_is_running = true;
	
	
	m_socket = new async::AsyncTcpSslSocket();
	event::TcpSslWaitEvent event(nullptr, m_socket);
	action::TcpSslEventAction action(&event);
	res = co_await m_socket->SSLSocket(&action, GetGlobalSSLCtx());
	m_socket->GetOption().HandleNonBlock();
	m_socket->GetOption().HandleReuseAddr();
	m_socket->GetOption().HandleReusePort();

	m_socket->BindAndListen(port, backlog);
	m_listen_events.resize(m_net_sche_num);
	for(int i = 0 ; i < m_net_sche_num; ++i )
	{
		m_listen_events[i] = new event::SslListenEvent(m_socket, store\
			, GetNetIOScheduler(i), GetCoroutineScheduler(i));
		GetNetIOScheduler(i)->AddEvent(m_listen_events[i]);
	} 
	co_return;
}
void TcpSslServer::Stop()
{
	if(!m_is_running) return;
	for(int i = 0 ; i < m_net_sche_num; ++i )
	{
		m_listen_events[i]->Free(GetNetIOScheduler(i)->GetEngine());
	}
	StopCoroutineSchedulers();
	StopNetIOSchedulers();
	m_is_running = false;
	DestroySSLEnv();
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void TcpSslServer::ReSetNetworkSchedulerNum(int num)
{
	if(m_is_running)
		DynamicResizeNetIOSchedulers(num);
	else 
		m_net_sche_num = num;
}

void TcpSslServer::ReSetCoroutineSchedulerNum(int num)
{
	if(m_is_running) 
		DynamicResizeCoroutineSchedulers(num);
	else
		m_co_sche_num = num;
}

async::AsyncTcpSslSocket *TcpSslServer::GetSocket()
{
    return m_socket;
}

TcpSslServer::~TcpSslServer()
{
	if( m_socket ) {
		delete m_socket;
	}
}
}