#include "Server.h"
#include "Event.h"
#include "Scheduler.h"
#include "Async.h"
#include "Operation.h"

namespace galay::server
{
TcpServer::TcpServer()
	: m_co_sche_num(DEFAULT_SERVER_CO_SCHEDULER_NUM), m_net_sche_num(DEFAULT_SERVER_NET_SCHEDULER_NUM), m_co_sche_timeout(-1)\
		, m_net_sche_timeout(-1), m_socket(nullptr), m_is_running(false)
{
}

coroutine::Coroutine TcpServer::Start(CallbackStore* store, int port, int backlog)
{
	
	scheduler::DynamicResizeCoroutineSchedulers(m_co_sche_num);
	scheduler::DynamicResizeNetIOSchedulers(m_net_sche_num);
	scheduler::StartCoroutineSchedulers(m_co_sche_timeout);
	scheduler::StartNetIOSchedulers(m_net_sche_timeout);
	m_is_running = true;
	
	
	m_socket = new async::AsyncTcpSocket();
	event::NetWaitEvent event(nullptr, m_socket);
	action::NetIoEventAction action(&event);
	bool res = co_await m_socket->InitialHandle(&action);
	m_socket->GetOption().HandleNonBlock();
	m_socket->GetOption().HandleReuseAddr();
	m_socket->GetOption().HandleReusePort();

	m_socket->BindAndListen(port, backlog);
	m_listen_events.resize(m_net_sche_num);
	for(int i = 0 ; i < m_net_sche_num; ++i )
	{
		m_listen_events[i] = new event::ListenEvent(m_socket, store\
			, scheduler::GetNetIOScheduler(i), scheduler::GetCoroutineScheduler(i));
		scheduler::GetNetIOScheduler(i)->AddEvent(m_listen_events[i]);
	} 
	co_return;
}

void TcpServer::Stop()
{
	for(int i = 0 ; i < m_net_sche_num; ++i )
	{
		m_listen_events[i]->Free(scheduler::GetNetIOScheduler(i)->GetEngine());
	}
	scheduler::StopCoroutineSchedulers();
	scheduler::StopNetIOSchedulers();
	m_is_running = false;
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void TcpServer::ReSetCoroutineSchedulerNum(int num)
{
	if(m_is_running) 
		scheduler::DynamicResizeCoroutineSchedulers(num);
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
		scheduler::DynamicResizeNetIOSchedulers(num);
	else 
		m_net_sche_num = num;
}

TcpServer::~TcpServer()
{
	if( m_socket ) {
		delete m_socket;
	}
}


}