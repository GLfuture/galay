#include "Server.h"
#include "Event.h"
#include "Scheduler.h"
#include "Async.h"
#include "Operation.h"

namespace galay::server
{
TcpServer::TcpServer()
	: m_co_sche_num(DEFAULT_SERVER_CO_SCHEDULER_NUM), m_net_sche_num(DEFAULT_SERVER_NET_SCHEDULER_NUM), m_co_sche_timeout(-1)\
		, m_net_sche_timeout(-1), m_socket(nullptr)
{
}

void TcpServer::Start(CallbackStore* store, int port, int backlog)
{
	
	scheduler::ResizeCoroutineSchedulers(m_co_sche_num);
	scheduler::ResizeNetIOSchedulers(m_net_sche_num);
	scheduler::StartCoroutineSchedulers(m_co_sche_timeout);
	scheduler::StartNetIOSchedulers(m_net_sche_timeout);
	m_socket = new async::AsyncTcpSocket();
	m_socket->InitialHandle();
	m_socket->GetOption().HandleNonBlock();
	m_socket->GetOption().HandleReuseAddr();
	m_socket->GetOption().HandleReusePort();

	m_socket->BindAndListen(port, backlog);
	m_listen_events.resize(m_net_sche_num);
	for(int i = 0 ; i < m_net_sche_num; ++i )
	{
		m_listen_events[i] = new event::ListenEvent(m_socket->GetHandle(), store\
			, scheduler::GetNetIOScheduler(i), scheduler::GetCoroutineScheduler(i));
		scheduler::GetNetIOScheduler(i)->AddEvent(m_listen_events[i]);
	} 
}

void TcpServer::Stop()
{
	for(int i = 0 ; i < m_net_sche_num; ++i )
	{
		m_listen_events[i]->Free(scheduler::GetNetIOScheduler(i)->GetEngine());
	}
	scheduler::StopCoroutineSchedulers();
	scheduler::StopNetIOSchedulers();
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void TcpServer::ReSetCoroutineSchedulerNum(int num)
{
	scheduler::ResizeCoroutineSchedulers(num);
}

async::AsyncTcpSocket *TcpServer::GetSocket()
{
    return m_socket;
}

void TcpServer::ReSetNetworkSchedulerNum(int num)
{
	scheduler::ResizeNetIOSchedulers(num);
}

TcpServer::~TcpServer()
{
	if( m_socket ) {
		delete m_socket;
	}
}


}