#include "Server.h"
#include "Event.h"
#include "Scheduler.h"
#include "Async.h"
#include "Operation.h"
#include "ExternApi.h"
#include "helper/HttpHelper.h"
#include <openssl/err.h>
#include <spdlog/spdlog.h>

#include <utility>

namespace galay::server
{

	
TcpServerConfig::TcpServerConfig()
	: m_backlog(DEFAULT_SERVER_BACKLOG)\
	, m_coroutine_scheduler_num(DEFAULT_SERVER_CO_SCHEDULER_NUM)\
	, m_network_scheduler_num(DEFAULT_SERVER_NET_SCHEDULER_NUM), m_network_scheduler_timeout_ms(DEFAULT_SERVER_NET_SCHEDULER_TIMEOUT_MS)\
	, m_event_scheduler_num(DEFAULT_SERVER_OTHER_SCHEDULER_NUM), m_event_scheduler_timeout_ms(DEFAULT_SERVER_OTHER_SCHEDULER_TIMEOUT_MS)\
	, m_timer_scheduler_num(DEFAULT_SERVER_TIMER_SCHEDULER_NUM), m_timer_scheduler_timeout_ms(DEFAULT_SERVER_TIMER_SCHEDULER_TIMEOUT_MS)
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


TcpServer::TcpServer(TcpServerConfig::ptr config)
{
	m_config = std::move(config);
}

void TcpServer::Start(TcpCallbackStore* store, const int port)
{
	
	DynamicResizeCoroutineSchedulers(m_config->m_coroutine_scheduler_num);
	DynamicResizeEventSchedulers(m_config->m_network_scheduler_num + m_config->m_event_scheduler_num);
	DynamicResizeTimerSchedulers(m_config->m_timer_scheduler_num);
	//coroutine scheduler
	StartCoroutineSchedulers();
	//net scheduler
	for(int i = 0 ; i < m_config->m_network_scheduler_num; ++i )
	{
		GetEventScheduler(i)->Loop(m_config->m_network_scheduler_timeout_ms);
	}
	//event scheduler
	for( int i = m_config->m_network_scheduler_num; i < m_config->m_network_scheduler_num + m_config->m_event_scheduler_num; ++i )
	{
		GetEventScheduler(i)->Loop(m_config->m_event_scheduler_timeout_ms);
	}
	StartTimerSchedulers(m_config->m_timer_scheduler_timeout_ms);
	m_is_running = true;
	m_listen_events.resize(m_config->m_network_scheduler_num);
	for(int i = 0 ; i < m_config->m_network_scheduler_num; ++i )
	{
		async::AsyncNetIo* socket = new async::AsyncTcpSocket(GetEventScheduler(i)->GetEngine());
		if(const bool res = AsyncSocket(socket); !res ) {
			delete socket;
			for(int j = 0; j < i; ++j ){
				delete m_listen_events[j];
			}
			m_listen_events.clear();
			spdlog::error("TcpServer::Start AsyncSocket failed");
			return;
		}
		async::HandleOption option(socket->GetHandle());
		option.HandleNonBlock();
		option.HandleReuseAddr();
		option.HandleReusePort();
		
		if(!BindAndListen(socket, port, m_config->m_backlog)) {
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
}

void TcpServer::Stop()
{
	if(!m_is_running) return;
	for(const auto & m_listen_event : m_listen_events)
	{
		delete m_listen_event;
	}
	StopCoroutineSchedulers();
	StopEventSchedulers();
	StopTimerSchedulers();
	Global_GetCoroutineStore()->Clear();
	m_is_running = false;
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TcpServer::~TcpServer()
= default;

TcpSslServer::TcpSslServer(const TcpSslServerConfig::ptr& config)
{
	m_config = config;
}

void TcpSslServer::Start(TcpSslCallbackStore *store, const int port)
{
    DynamicResizeCoroutineSchedulers(m_config->m_coroutine_scheduler_num);
	DynamicResizeEventSchedulers(m_config->m_network_scheduler_num + m_config->m_event_scheduler_num);
	DynamicResizeTimerSchedulers(m_config->m_timer_scheduler_num);
	//coroutine scheduler
	StartCoroutineSchedulers();
	//net scheduler
	for(int i = 0 ; i < m_config->m_network_scheduler_num; ++i )
	{
		GetEventScheduler(i)->Loop(m_config->m_network_scheduler_timeout_ms);
	}
	//event scheduler
	for( int i = m_config->m_network_scheduler_num; i < m_config->m_network_scheduler_num + m_config->m_event_scheduler_num; ++i )
	{
		GetEventScheduler(i)->Loop(m_config->m_event_scheduler_timeout_ms);
	}
	StartTimerSchedulers(m_config->m_timer_scheduler_timeout_ms);
    if(const bool res = InitialSSLServerEnv(m_config->m_cert_file, m_config->m_key_file); !res ) {
		spdlog::error("InitialSSLServerEnv failed, error:{}", ERR_error_string(ERR_get_error(), nullptr));
		return;
	}
	m_is_running = true;
	
	m_listen_events.resize(m_config->m_network_scheduler_num);
	for(int i = 0 ; i < m_config->m_network_scheduler_num; ++i )
	{
		async::AsyncSslNetIo* socket = new async::AsyncSslTcpSocket(GetEventScheduler(i)->GetEngine());
		bool res = AsyncSocket(socket);
		if( !res ) {
			delete socket;
			for(int j = 0; j < i; ++j ){
				delete m_listen_events[j];
			}
			m_listen_events.clear();
			spdlog::error("TcpSslServer::Start AsyncSocket failed");
			return;
		}
		async::HandleOption option(socket->GetHandle());
		option.HandleNonBlock();
		option.HandleReuseAddr();
		option.HandleReusePort();
		res = AsyncSSLSocket(socket, GetGlobalSSLCtx());
		if( !res ) {
			delete socket;
			for(int j = 0; j < i; ++j ){
				delete m_listen_events[j];
			}
			m_listen_events.clear();
			spdlog::error("TcpSslServer::Start AsyncSSLSocket failed");
			return;
		}
		
		if(!BindAndListen(socket, port, m_config->m_backlog)) {
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
}

void TcpSslServer::Stop()
{
	if(!m_is_running) return;
	for(const auto & m_listen_event : m_listen_events)
	{
		delete m_listen_event;
	}
	StopCoroutineSchedulers();
	StopEventSchedulers();
	StopTimerSchedulers();
	Global_GetCoroutineStore()->Clear();
	m_is_running = false;
	DestroySSLEnv();
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TcpSslServer::~TcpSslServer()
= default;

//HttpServer

ServerProtocolStore<protocol::http::HttpRequest, protocol::http::HttpResponse> HttpServer::g_http_proto_store;
protocol::http::HttpResponse HttpServer::g_method_not_allowed_resp;
protocol::http::HttpResponse HttpServer::g_uri_too_long_resp;
protocol::http::HttpResponse HttpServer::g_not_found_resp;

HttpServer::HttpServer(const HttpServerConfig::ptr& config)
	: TcpServer(config), m_store(std::make_unique<TcpCallbackStore>([this](const TcpOperation& operation)->coroutine::Coroutine {
		return HttpRoute(operation);
	}))
{
	g_http_proto_store.Initial(config->m_proto_capacity);
	helper::http::HttpHelper::DefaultNotFound(&g_not_found_resp);
	helper::http::HttpHelper::DefaultUriTooLong(&g_uri_too_long_resp);
	helper::http::HttpHelper::DefaultMethodNotAllowed(&g_method_not_allowed_resp);
}

void HttpServer::ReturnResponse(protocol::http::HttpResponse *response)
{
	g_http_proto_store.ReturnResponse(response);
}

void HttpServer::ReturnRequest(protocol::http::HttpRequest *request)
{
	g_http_proto_store.ReturnRequest(request);
}

protocol::http::HttpResponse& HttpServer::GetDefaultMethodNotAllowedResponse()
{
    return g_method_not_allowed_resp;
}

protocol::http::HttpResponse &HttpServer::GetDefaultUriTooLongResponse()
{
	return g_uri_too_long_resp;
}

protocol::http::HttpResponse &HttpServer::GetDefaultNotFoundResponse()
{
	return g_not_found_resp;
}

void HttpServer::Get(const std::string &path, std::function<coroutine::Coroutine(HttpOperation)> &&handler)
{
	m_route_map[protocol::http::HttpMethod::Http_Method_Get][path] = std::forward<std::function<coroutine::Coroutine(HttpOperation)>>(handler);
}

void HttpServer::Start(int port)
{
	TcpServer::Start(m_store.get(), port);
	return;
}

void HttpServer::Stop()
{
	TcpServer::Stop();
}

coroutine::Coroutine HttpServer::HttpRoute(TcpOperation operation)
{
	std::weak_ptr<HttpServerConfig> config = std::dynamic_pointer_cast<HttpServerConfig>(m_config);
	auto connection = operation.GetConnection();
	HttpOperation http_operaion(operation, g_http_proto_store.GetRequest(), g_http_proto_store.GetResponse());
	char* buffer = static_cast<char*>(malloc(config.lock()->m_max_header_size));
	size_t total_length = 0;
	IOVec vec {
		.m_buffer = buffer,
		.m_length = config.lock()->m_max_header_size
	};
	int length = co_await AsyncRecv(connection->GetSocket(), &vec);
	if(length <= 0) {
		free(vec.m_buffer);
		co_await AsyncClose(connection->GetSocket());
		co_return;
	} else {
		total_length += length;
		std::string_view data(buffer, total_length);
		auto result = http_operaion.GetRequest()->DecodePdu(data);
		if( result.first == false ) {
			switch (http_operaion.GetRequest()->GetErrorCode())
			{
			case error::HttpErrorCode::kHttpError_BodyInComplete:
			case error::HttpErrorCode::kHttpError_HeaderInComplete:
				break;
			//case error::

			//default:
				break;
			}
		} else {
			
		}
	}
    co_return;
}



}