#include "Server.h"
#include "Event.h"
#include "Scheduler.h"
#include "Async.h"
#include "Operation.h"
#include "ExternApi.h"
#include <openssl/err.h>
#include "spdlog/spdlog.h"

namespace galay::server
{

	
TcpServerConfig::TcpServerConfig()
	: m_backlog(DEFAULT_SERVER_BACKLOG)\
	, m_coroutine_scheduler_num(DEFAULT_SERVER_CO_SCHEDULER_NUM)\
	, m_network_scheduler_num(DEFAULT_SERVER_NET_SCHEDULER_NUM), m_network_scheduler_timeout_ms(DEFAULT_SERVER_NET_SCHEDULER_TIMEOUT_MS)\
	, m_event_scheduler_num(DEFAULT_SERVER_OTHER_SCHEDULER_NUM), m_event_scheduler_timeout_ms(DEFAULT_SERVER_OTHER_SCHEDULER_TIMEOUT_MS)
{
}

TcpSslServerConfig::TcpSslServerConfig()
	:m_cert_file(nullptr), m_key_file(nullptr)
{
}

TcpServer::TcpServer()
	: m_is_running(false)
{
}

TcpServer::TcpServer(const TcpServerConfig &config)
{
	m_config = config;
}

void TcpServer::Start(TcpCallbackStore* store, int port)
{
	
	DynamicResizeCoroutineSchedulers(m_config.m_coroutine_scheduler_num);
	DynamicResizeEventSchedulers(m_config.m_network_scheduler_num + m_config.m_event_scheduler_num);
	//coroutine scheduler
	StartCoroutineSchedulers();
	//net scheduler
	for(int i = 0 ; i < m_config.m_network_scheduler_num; ++i )
	{
		GetEventScheduler(i)->Loop(m_config.m_network_scheduler_timeout_ms);
	}
	//event scheduler
	for( int i = m_config.m_network_scheduler_num; i < m_config.m_network_scheduler_num + m_config.m_event_scheduler_num; ++i )
	{
		GetEventScheduler(i)->Loop(m_config.m_event_scheduler_timeout_ms);
	}
	m_is_running = true;
	m_listen_events.resize(m_config.m_network_scheduler_num);
	for(int i = 0 ; i < m_config.m_network_scheduler_num; ++i )
	{
		GHandle handle = async::AsyncTcpSocket::Socket();
		async::HandleOption option(handle);
		option.HandleNonBlock();
		option.HandleReuseAddr();
		option.HandleReusePort();
		async::AsyncTcpSocket* socket = new async::AsyncTcpSocket(handle);
		if(!socket->BindAndListen(port, m_config.m_backlog)) {
			spdlog::error("BindAndListen failed, error:{}", error::GetErrorString(socket->GetErrorCode()));
			delete socket;
			for(int j = 0; j < i; ++j ){
				delete m_listen_events[j];
			}
			m_listen_events.clear();
			return;
		}
		m_listen_events[i] = new event::ListenEvent(GetEventScheduler(i)->GetEngine(), socket, store);
	} 
	return;
}

void TcpServer::Stop()
{
	if(!m_is_running) return;
	for(int i = 0 ; i < m_listen_events.size(); ++i )
	{
		delete m_listen_events[i];
	}
	StopCoroutineSchedulers();
	StopEventSchedulers();
	m_is_running = false;
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TcpServer::~TcpServer()
{
}

TcpSslServer::TcpSslServer(const char* cert_file, const char* key_file)
	: m_config(), m_is_running(false)
{
	m_config.m_cert_file = cert_file;
	m_config.m_key_file = key_file;
}

TcpSslServer::TcpSslServer(const TcpSslServerConfig &config)
{
	m_config = config;
}

void TcpSslServer::Start(TcpSslCallbackStore *store, int port)
{
    DynamicResizeCoroutineSchedulers(m_config.m_coroutine_scheduler_num);
	DynamicResizeEventSchedulers(m_config.m_network_scheduler_num + m_config.m_event_scheduler_num);
	//coroutine scheduler
	StartCoroutineSchedulers();
	//net scheduler
	for(int i = 0 ; i < m_config.m_network_scheduler_num; ++i )
	{
		GetEventScheduler(i)->Loop(m_config.m_network_scheduler_timeout_ms);
	}
	//event scheduler
	for( int i = m_config.m_network_scheduler_num; i < m_config.m_network_scheduler_num + m_config.m_event_scheduler_num; ++i )
	{
		GetEventScheduler(i)->Loop(m_config.m_event_scheduler_timeout_ms);
	}
	
	bool res = InitialSSLServerEnv(m_config.m_cert_file, m_config.m_key_file);
	if( !res ) {
		spdlog::error("InitialSSLServerEnv failed, error:{}", ERR_error_string(ERR_get_error(), nullptr));
		return;
	}
	m_is_running = true;
	
	m_listen_events.resize(m_config.m_network_scheduler_num);
	for(int i = 0 ; i < m_config.m_network_scheduler_num; ++i )
	{
		GHandle handle = async::AsyncTcpSocket::Socket();
		async::HandleOption option(handle);
		option.HandleNonBlock();
		option.HandleReuseAddr();
		option.HandleReusePort();
		SSL* ssl = async::AsyncTcpSslSocket::SSLSocket(handle, GetGlobalSSLCtx());
		if(ssl == nullptr) {
			exit(-1);
		}
		async::AsyncTcpSslSocket* socket = new async::AsyncTcpSslSocket(handle, ssl);
		if(!socket->BindAndListen(port, m_config.m_backlog)) {
			spdlog::error("BindAndListen failed, error:{}", error::GetErrorString(socket->GetErrorCode()));
			delete socket;
			for(int j = 0; j < i; ++j ){
				delete m_listen_events[j];
			}
			m_listen_events.clear();
			return;
		}
		m_listen_events[i] = new event::SslListenEvent(GetEventScheduler(i)->GetEngine(), socket, store);
	} 
	return;
}
void TcpSslServer::Stop()
{
	if(!m_is_running) return;
	for(int i = 0 ; i < m_listen_events.size(); ++i )
	{
		delete m_listen_events[i];
	}
	StopCoroutineSchedulers();
	StopEventSchedulers();
	m_is_running = false;
	DestroySSLEnv();
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TcpSslServer::~TcpSslServer()
{
}

//HttpServer

ServerProtocolStore<protocol::http::HttpRequest, protocol::http::HttpResponse> HttpServer::g_http_proto_store;
std::unordered_map<std::string, std::unordered_map<std::string, std::function<coroutine::Coroutine()>>> HttpServer::m_route_map;

HttpServer::HttpServer(uint32_t proto_capacity)
	: m_store(std::make_unique<TcpCallbackStore>(HttpRoute))
{
	g_http_proto_store.Initial(proto_capacity);
}

void HttpServer::Get(const std::string &path, std::function<coroutine::Coroutine()> &&handler)
{
	m_route_map["GET"][path] = std::forward<std::function<coroutine::Coroutine()>>(handler);
}

void HttpServer::Start(int port)
{
	
}

coroutine::Coroutine HttpServer::HttpRoute(TcpOperation operation)
{
    auto connection = operation.GetConnection();
	int length = co_await connection->WaitForRecv();
	if( length == 0 ) {
		auto data = connection->FetchRecvData();
		data.Clear();
		bool b = co_await connection->CloseConnection();
		co_return;
	} else {
		auto data = connection->FetchRecvData();
		auto request = g_http_proto_store.GetRequest();
		int elength = request->DecodePdu(data.Data());
		if(request->HasError()) {
			if( request->GetErrorCode() == galay::error::HttpErrorCode::kHttpError_HeaderInComplete || request->GetErrorCode() == galay::error::HttpErrorCode::kHttpError_BodyInComplete)
			{
				if( elength >= 0 ) {
					data.Erase(elength);
				} 
				operation.GetContext() = request;
				operation.ReExecute(operation);
			} else {
				data.Clear();
				bool b = co_await connection->CloseConnection();
				co_return;
			}
		}
		else{
			data.Clear();
			// TODO : handle request
			// Note: close connection
		}
	}
	co_return;
}
}