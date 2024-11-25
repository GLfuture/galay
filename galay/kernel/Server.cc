#include "Server.h"
#include "Event.h"
#include "Scheduler.h"
#include "Async.h"
#include "Operation.h"
#include "ExternApi.h"
#include "helper/HttpHelper.h"
#include <openssl/err.h>
#include <spdlog/spdlog.h>

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
	DynamicResizeTimerSchedulers(m_config.m_timer_scheduler_num);
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
	StartTimerSchedulers(m_config.m_timer_scheduler_timeout_ms);
	m_is_running = true;
	m_listen_events.resize(m_config.m_network_scheduler_num);
	for(int i = 0 ; i < m_config.m_network_scheduler_num; ++i )
	{
		async::AsyncNetIo* socket = new async::AsyncTcpSocket(GetEventScheduler(i)->GetEngine());
		bool res = async::AsyncSocket(socket);
		if( !res ) {
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
		
		if(!async::BindAndListen(socket, port, m_config.m_backlog)) {
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
	StopTimerSchedulers();
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
	DynamicResizeTimerSchedulers(m_config.m_timer_scheduler_num);
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
	StartTimerSchedulers(m_config.m_timer_scheduler_timeout_ms);
	bool res = InitialSSLServerEnv(m_config.m_cert_file, m_config.m_key_file);
	if( !res ) {
		spdlog::error("InitialSSLServerEnv failed, error:{}", ERR_error_string(ERR_get_error(), nullptr));
		return;
	}
	m_is_running = true;
	
	m_listen_events.resize(m_config.m_network_scheduler_num);
	for(int i = 0 ; i < m_config.m_network_scheduler_num; ++i )
	{
		async::AsyncSslNetIo* socket = new async::AsyncSslTcpSocket(GetEventScheduler(i)->GetEngine());
		bool res = async::AsyncSocket(socket);
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
		res = async::AsyncSSLSocket(socket, GetGlobalSSLCtx());
		if( !res ) {
			delete socket;
			for(int j = 0; j < i; ++j ){
				delete m_listen_events[j];
			}
			m_listen_events.clear();
			spdlog::error("TcpSslServer::Start AsyncSSLSocket failed");
			return;
		}
		
		if(!async::BindAndListen(socket, port, m_config.m_backlog)) {
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
	StopTimerSchedulers();
	m_is_running = false;
	DestroySSLEnv();
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TcpSslServer::~TcpSslServer()
{
}

//HttpServer

ServerProtocolStore<protocol::http::HttpRequest, protocol::http::HttpResponse> HttpServer::g_http_proto_store;
std::unordered_map<protocol::http::HttpMethod, std::unordered_map<std::string, std::function<coroutine::Coroutine(HttpOperation)>>> HttpServer::m_route_map;
protocol::http::HttpResponse HttpServer::g_method_not_allowed_resp;
protocol::http::HttpResponse HttpServer::g_uri_too_long_resp;
protocol::http::HttpResponse HttpServer::g_not_found_resp;

HttpServer::HttpServer(uint32_t proto_capacity)
	: m_store(std::make_unique<TcpCallbackStore>(HttpRoute))
{
	g_http_proto_store.Initial(proto_capacity);
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
//     auto connection = operation.GetConnection();
// 	async::NetIOVec iovec {
// 		.m_buf = new char[DEFAULT_READ_BUFFER_SIZE],
// 		.m_len = DEFAULT_READ_BUFFER_SIZE
// 	};
// 	int length = co_await async::AsyncRecv(connection->GetSocket(), &iovec);
// 	if( length == 0 ) {
// 		if(operation.GetContext().has_value()) {
// 			auto request = std::any_cast<protocol::http::HttpRequest*>(operation.GetContext());
// 			ReturnRequest(request);
// 		}
// 		co_return;
// 	} else {
// 		protocol::http::HttpRequest* request = nullptr;
// 		if(!operation.GetContext().has_value()) request = g_http_proto_store.GetRequest();
// 		else request = std::any_cast<protocol::http::HttpRequest*>(operation.GetContext());
// 		auto data = connection->FetchRecvData();
// 		int elength = request->DecodePdu(data.Data());
// 		if(request->HasError()) {
// 			if( request->GetErrorCode() == galay::error::HttpErrorCode::kHttpError_HeaderInComplete || request->GetErrorCode() == galay::error::HttpErrorCode::kHttpError_BodyInComplete)
// 			{
// 				if( elength >= 0 ) {
// 					data.Erase(elength);
// 				} 
// 				operation.GetContext() = request;
// 				operation.ReExecute(operation);
// 				co_return;
// 			} else {
// 				data.Clear();
// 				if( request->GetErrorCode() == galay::error::HttpErrorCode::kHttpError_UriTooLong ){
// 					std::string str = g_uri_too_long_resp.EncodePdu();
// 					connection->PrepareSendData(str);
// 					int ret = co_await connection->WaitForSend();
// 				}
// 				operation.GetContext().reset();
// 				g_http_proto_store.ReturnRequest(request);
// 				bool b = co_await connection->CloseConnection();
// 				co_return;
// 			}
// 		}
// 		else{
// 			data.Clear();
// 			auto it = m_route_map.find(request->Header()->Method());
// 			if( it != m_route_map.end()){
// 				auto inner_it = it->second.find(request->Header()->Uri());
// 				if (inner_it != it->second.end()) {
// 					auto response = g_http_proto_store.GetResponse();
// 					HttpOperation httpop(operation, request, response);
// 					inner_it->second(httpop);
// 				} else {
// 					std::string str = g_not_found_resp.EncodePdu();
// 					connection->PrepareSendData(str);
// 					int ret = co_await connection->WaitForSend();
// 					g_http_proto_store.ReturnRequest(request);
// 					bool b = co_await connection->CloseConnection();
// 					co_return;
// 				}
// 			} else {
// 				std::string str = g_method_not_allowed_resp.EncodePdu();
// 				connection->PrepareSendData(str);
// 				int ret = co_await connection->WaitForSend();
// 				g_http_proto_store.ReturnRequest(request);
// 				bool b = co_await connection->CloseConnection();
// 				co_return;
// 			}
			
// 		}
// 	}
	
// 	co_return;
}


}