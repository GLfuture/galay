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
		if(const bool res = async::AsyncSocket(socket); !res ) {
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
		
		if(!async::BindAndListen(socket, port, m_config->m_backlog)) {
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
	GetThisThreadCoroutineStore()->Clear();
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
		
		if(!async::BindAndListen(socket, port, m_config->m_backlog)) {
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
	GetThisThreadCoroutineStore()->Clear();
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
    // auto connection = operation.GetConnection();
	// auto request = g_http_proto_store.GetRequest();
	// auto response = g_http_proto_store.GetResponse();
	// HttpOperation httpop(operation, request, response);
	// size_t max_header_len = std::dynamic_pointer_cast<HttpServerConfig>(m_config)->m_max_header_size;
	// char* buffer = (char*)malloc(max_header_len);
	// int total_length = 0;
	// async::NetIOVec iovec {
	// 	.m_buf = buffer,
	// 	.m_len = max_header_len
	// };
	// int elength = 0;
	// while(1) {
	// 	int length = co_await async::AsyncRecv(connection->GetSocket(), &iovec);
	// 	if( length == 0 ) {
	// 		break;
	// 	} else {
	// 		total_length += length;
	// 		// int ret = request->DecodePdu(std::string_view(buffer + elength, total_length - elength));
	// 		if( ret > 0) elength += ret;
	// 		if(request->HasError()) {
	// 			if( request->GetErrorCode() == galay::error::HttpErrorCode::kHttpError_HeaderInComplete && ret >= max_header_len) 
	// 			{
	// 				//header too long
					
	// 			}
	// 			else if(request->GetErrorCode() == galay::error::HttpErrorCode::kHttpError_BodyInComplete)
	// 			{
	// 				int body_len = 0;
	// 				try
	// 				{
	// 					do{
	// 						std::string temp = request->Header()->HeaderPairs().GetValue("Content-Length");
	// 						if(!temp.empty()) {
	// 							body_len = std::stoi(temp);
	// 							break;
	// 						}
	// 						temp = request->Header()->HeaderPairs().GetValue("Content-Length");
	// 						if(!temp.empty()) {
	// 							body_len = std::stoi(temp);
	// 							break;
	// 						}
	// 						body_len = DEFAULT_READ_BUFFER_SIZE;
	// 					}while(0);
	// 				}
	// 				catch(const std::exception& e)
	// 				{
	// 					spdlog::error("{}", e.what());
	// 					break;
	// 				}
	// 				buffer = (char*)realloc(buffer, elength + body_len);
	// 				iovec.m_buf = buffer + elength;
	// 				iovec.m_len = body_len;
	// 			}
	// 			else if( request->GetErrorCode() == galay::error::HttpErrorCode::kHttpError_UriTooLong ) {
	// 				auto resp = g_uri_too_long_resp.EncodePdu();
	// 				async::NetIOVec wiov {
	// 					.m_buf = resp.data(),
	// 					.m_len = resp.length()
	// 				};
	// 				int sendlen = co_await async::AsyncSend(connection->GetSocket(), &wiov);
	// 				break;
	// 			}else{
	// 				break;
	// 			}
	// 		}else{
	// 			auto it = m_route_map.find(request->Header()->Method());
	// 			if( it != m_route_map.end()){
	// 				auto inner_it = it->second.find(request->Header()->Uri());
	// 				if (inner_it != it->second.end()) {
	// 					inner_it->second(httpop);
	// 				} else {
	// 					auto resp = g_not_found_resp.EncodePdu();
	// 					async::NetIOVec wiov {
	// 						.m_buf = resp.data(),
	// 						.m_len = resp.length()
	// 					};
	// 					int sendlen = co_await async::AsyncSend(connection->GetSocket(), &wiov);
	// 					break;
	// 				}
	// 			} else {
	// 				auto resp = g_method_not_allowed_resp.EncodePdu();
	// 				async::NetIOVec wiov {
	// 					.m_buf = resp.data(),
	// 					.m_len = resp.length()
	// 				};
	// 				int sendlen = co_await async::AsyncSend(connection->GetSocket(), &wiov);
	// 				break;
	// 			}

	// 		}
	// 	}
	// 	free(iovec.m_buf);
	// 	co_await async::AsyncClose(connection->GetSocket());
	// 	co_return;
	}
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
//}


}