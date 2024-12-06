#include "Server.h"
#include "Event.h"
#include "Scheduler.h"
#include "Async.h"
#include "Connection.h"
#include "ExternApi.h"
#include "galay/helper/HttpHelper.h"
#include "Log.h"
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

TcpServer::TcpServer(TcpServerConfig::ptr config)
{
	m_config = std::move(config);
}

void TcpServer::Start(TcpCallbackStore* store, const int port)
{
	
	InitializeGalayEnv(m_config->m_coroutineConf, m_config->m_netSchedulerConf, m_config->m_coroutineConf);
	m_is_running = true;
	m_listen_events.resize(m_config->m_netSchedulerConf.first);
	for(int i = 0 ; i < m_config->m_netSchedulerConf.first; ++i )
	{
		async::AsyncNetIo* socket = new async::AsyncTcpSocket(EeventSchedulerHolder::GetInstance()->GetScheduler(i)->GetEngine());
		if(const bool res = AsyncSocket(socket); !res ) {
			delete socket;
			for(int j = 0; j < i; ++j ){
				delete m_listen_events[j];
			}
			m_listen_events.clear();
			LogError("[Socket Init failed, error:{}]", error::GetErrorString(socket->GetErrorCode()));
			return;
		}
		async::HandleOption option(socket->GetHandle());
		option.HandleNonBlock();
		option.HandleReuseAddr();
		option.HandleReusePort();
		
		if(!BindAndListen(socket, port, m_config->m_backlog)) {
			LogError("[BindAndListen failed, error:{}]", error::GetErrorString(socket->GetErrorCode()));
			delete socket;
			for(int j = 0; j < i; ++j ){
				delete m_listen_events[j];
			}
			m_listen_events.clear();
			return;
		}
		m_listen_events[i] = new details::ListenEvent(EeventSchedulerHolder::GetInstance()->GetScheduler(i)->GetEngine(), socket, store);
	} 
}

void TcpServer::Stop()
{
	if(!m_is_running) return;
	for(const auto & m_listen_event : m_listen_events)
	{
		delete m_listen_event;
	}
	DestroyGalayEnv();
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
    InitializeGalayEnv(m_config->m_coroutineConf, m_config->m_netSchedulerConf, m_config->m_coroutineConf);
    if(const bool res = InitializeSSLServerEnv(m_config->m_cert_file, m_config->m_key_file); !res ) {
		LogError("[InitializeSSLServerEnv init failed, error:{}]", ERR_error_string(ERR_get_error(), nullptr));
		return;
	}
	m_is_running = true;
	
	m_listen_events.resize(m_config->m_netSchedulerConf.first);
	for(int i = 0 ; i < m_config->m_netSchedulerConf.first; ++i )
	{
		async::AsyncSslNetIo* socket = new async::AsyncSslTcpSocket(EeventSchedulerHolder::GetInstance()->GetScheduler(i)->GetEngine());
		bool res = AsyncSocket(socket);
		if( !res ) {
			delete socket;
			for(int j = 0; j < i; ++j ){
				delete m_listen_events[j];
			}
			m_listen_events.clear();
			LogError("[Socket init failed, error: {}]", error::GetErrorString(socket->GetErrorCode()));
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
			LogError("[SSLSocket init failed, error: {}]", error::GetErrorString(socket->GetErrorCode()));
			return;
		}
		
		if(!BindAndListen(socket, port, m_config->m_backlog)) {
			LogError("[BindAndListen failed, error:{}]", error::GetErrorString(socket->GetErrorCode()));
			delete socket;
			for(int j = 0; j < i; ++j ){
				delete m_listen_events[j];
			}
			m_listen_events.clear();
			return;
		}
		m_listen_events[i] = new details::SslListenEvent(EeventSchedulerHolder::GetInstance()->GetScheduler(i)->GetEngine(), socket, store);
	} 
}

void TcpSslServer::Stop()
{
	if(!m_is_running) return;
	for(const auto & m_listen_event : m_listen_events)
	{
		delete m_listen_event;
	}
	DestroyGalayEnv();
	m_is_running = false;
	DestroySSLEnv();
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TcpSslServer::~TcpSslServer()
= default;

//HttpServer

util::ObjectPoolMutiThread<HttpServer::HttpRequest> HttpServer::RequestPool(DEFAULT_HTTP_REQUEST_POOL_SIZE);
util::ObjectPoolMutiThread<HttpServer::HttpResponse> HttpServer::ResponsePool(DEFAULT_HTTP_RESPONSE_POOL_SIZE);

HttpServer::HttpServer(const HttpServerConfig::ptr& config)
	: m_server(config), m_store(std::make_unique<TcpCallbackStore>([this](const TcpConnectionManager& manager)->coroutine::Coroutine {
		return HttpRoute(manager);
	}))
{
}

void HttpServer::Get(const std::string &path, std::function<coroutine::Coroutine(HttpConnectionManager, RoutineContext_ptr)> &&handler)
{
	m_route_map[protocol::http::HttpMethod::Http_Method_Get][path] = std::forward<std::function<coroutine::Coroutine(HttpConnectionManager, RoutineContext_ptr)>>(handler);
}

void HttpServer::Post(const std::string &path, std::function<Coroutine(HttpConnectionManager, RoutineContext_ptr)> &&handler)
{
	m_route_map[protocol::http::HttpMethod::Http_Method_Post][path] = std::forward<std::function<coroutine::Coroutine(HttpConnectionManager, RoutineContext_ptr)>>(handler);
}

void HttpServer::Put(const std::string &path, std::function<Coroutine(HttpConnectionManager, RoutineContext_ptr)> &&handler)
{
	m_route_map[protocol::http::HttpMethod::Http_Method_Put][path] = std::forward<std::function<coroutine::Coroutine(HttpConnectionManager, RoutineContext_ptr)>>(handler);
}

void HttpServer::Delete(const std::string &path, std::function<Coroutine(HttpConnectionManager, RoutineContext_ptr)> &&handler)
{
	m_route_map[protocol::http::HttpMethod::Http_Method_Delete][path] = std::forward<std::function<coroutine::Coroutine(HttpConnectionManager, RoutineContext_ptr)>>(handler);
}

void HttpServer::Options(const std::string &path, std::function<Coroutine(HttpConnectionManager, RoutineContext_ptr)> &&handler)
{
	m_route_map[protocol::http::HttpMethod::Http_Method_Options][path] = std::forward<std::function<coroutine::Coroutine(HttpConnectionManager, RoutineContext_ptr)>>(handler);
}

void HttpServer::Head(const std::string &path, std::function<Coroutine(HttpConnectionManager, RoutineContext_ptr)> &&handler)
{
	m_route_map[protocol::http::HttpMethod::Http_Method_Head][path] = std::forward<std::function<coroutine::Coroutine(HttpConnectionManager, RoutineContext_ptr)>>(handler);
}

void HttpServer::Trace(const std::string &path, std::function<Coroutine(HttpConnectionManager, RoutineContext_ptr)> &&handler)
{
	m_route_map[protocol::http::HttpMethod::Http_Method_Trace][path] = std::forward<std::function<coroutine::Coroutine(HttpConnectionManager, RoutineContext_ptr)>>(handler);
}

void HttpServer::Patch(const std::string &path, std::function<Coroutine(HttpConnectionManager, RoutineContext_ptr)> &&handler)
{
	m_route_map[protocol::http::HttpMethod::Http_Method_Patch][path] = std::forward<std::function<coroutine::Coroutine(HttpConnectionManager, RoutineContext_ptr)>>(handler);
}

void HttpServer::Connect(const std::string &path, std::function<Coroutine(HttpConnectionManager, RoutineContext_ptr)> &&handler)
{
	m_route_map[protocol::http::HttpMethod::Http_Method_Connect][path] = std::forward<std::function<coroutine::Coroutine(HttpConnectionManager, RoutineContext_ptr)>>(handler);
}

void HttpServer::Start(int port)
{
	m_server.Start(m_store.get(), port);
	return;
}

void HttpServer::Stop()
{
	m_server.Stop();
}

/*
	Keep-alive 需要处理
*/
coroutine::Coroutine HttpServer::HttpRoute(TcpConnectionManager manager)
{
	co_await this_coroutine::AddToCoroutineStore();
	size_t max_header_size = std::dynamic_pointer_cast<HttpServerConfig>(m_server.GetConfig())->m_max_header_size;
	auto connection = manager.GetConnection();
	async::AsyncNetIo* socket = connection->GetSocket();
	IOVecHolder rholder(max_header_size), wholder;
	HttpConnectionManager http_manager(manager, &RequestPool, &ResponsePool);
	auto [context, cancel] = coroutine::ContextFactory::WithWaitGroupContext();
	co_await this_coroutine::Exit([cancel, socket](){
		(*cancel)();
	});
	bool close_connection = false;
	while(true)
	{
step1:
		while(true) {
			int length = co_await AsyncRecv(socket, &rholder, max_header_size);
			if( length <= 0 ) {
				if( length == details::CommonFailedType::eCommonDisConnect || length == details::CommonFailedType::eCommonOtherFailed ) {
					bool res = co_await AsyncClose(socket);
					co_return;
				}
			} 
			else { 
				std::string_view data(rholder->m_buffer, rholder->m_offset);
				auto result = http_manager.GetRequest()->DecodePdu(data);
				if(!result.first) { //解析失败
					switch (http_manager.GetRequest()->GetErrorCode())
					{
					case error::HttpErrorCode::kHttpError_HeaderInComplete:
					case error::HttpErrorCode::kHttpError_BodyInComplete:
					{
						if( rholder->m_offset >= rholder->m_size) {
							rholder.Realloc(rholder->m_size * 2);
						}
						break;
					}
					case galay::error::HttpErrorCode::kHttpError_BadRequest:
					{
						if(m_error_string.contains(HttpStatusCode::BadRequest_400)) {
							std::string body = m_error_string[HttpStatusCode::BadRequest_400];
							CreateHttpResponse(http_manager.GetResponse(), HttpVersion::Http_Version_1_1, HttpStatusCode::BadRequest_400, std::move(body));
						} else {
							std::string body = "Bad Request";
							CreateHttpResponse(http_manager.GetResponse(), HttpVersion::Http_Version_1_1, HttpStatusCode::BadRequest_400, std::move(body));
						}
						wholder.Reset(http_manager.GetResponse()->EncodePdu());
						close_connection = true;
						goto step2;
					}
					case galay::error::HttpErrorCode::kHttpError_HeaderTooLong:
					{
						if(m_error_string.contains(HttpStatusCode::RequestHeaderFieldsTooLarge_431)) {
							std::string body = m_error_string[HttpStatusCode::RequestHeaderFieldsTooLarge_431];
							CreateHttpResponse(http_manager.GetResponse(), HttpVersion::Http_Version_1_1, HttpStatusCode::RequestHeaderFieldsTooLarge_431, std::move(body));
						} else {
							std::string body = "Header Too Long";
							CreateHttpResponse(http_manager.GetResponse(), HttpVersion::Http_Version_1_1, HttpStatusCode::RequestHeaderFieldsTooLarge_431, std::move(body));
						}
						wholder.Reset(http_manager.GetResponse()->EncodePdu());
						close_connection = true;
						goto step2;
					}
					case galay::error::HttpErrorCode::kHttpError_UriTooLong:
					{
						if(m_error_string.contains(HttpStatusCode::UriTooLong_414)) {
							std::string body = m_error_string[HttpStatusCode::UriTooLong_414];
							CreateHttpResponse(http_manager.GetResponse(), HttpVersion::Http_Version_1_1, HttpStatusCode::UriTooLong_414, std::move(body));
						} else {
							std::string body = "Uri Too Long";
							CreateHttpResponse(http_manager.GetResponse(), HttpVersion::Http_Version_1_1, HttpStatusCode::UriTooLong_414, std::move(body));
						}
						wholder.Reset(http_manager.GetResponse()->EncodePdu());
						close_connection = true;
						goto step2;
					}
					default:
						break;
					}
				}
				else { //解析成功
					auto it = m_route_map.find(http_manager.GetRequest()->Header()->Method());
					if(it != m_route_map.end()) {
						auto it2 = it->second.find(http_manager.GetRequest()->Header()->Uri());
						if(it2 != it->second.end()) {
							it2->second(http_manager, context);
							wholder.Reset(http_manager.GetResponse()->EncodePdu());
							goto step2;
						} else { //没有该URI
					 		if(m_error_string.contains(HttpStatusCode::NotFound_404)) {
								std::string body = m_error_string[HttpStatusCode::NotFound_404];
								CreateHttpResponse(http_manager.GetResponse(), HttpVersion::Http_Version_1_1, HttpStatusCode::NotFound_404, std::move(body));
							} else {
								std::string body = "Not Found";
								CreateHttpResponse(http_manager.GetResponse(), HttpVersion::Http_Version_1_1, HttpStatusCode::NotFound_404, std::move(body));
							}
							wholder.Reset(http_manager.GetResponse()->EncodePdu());
							goto step2;
						}
					} else { //没有该方法
						if(m_error_string.contains(HttpStatusCode::MethodNotAllowed_405)) {
							std::string body = m_error_string[HttpStatusCode::MethodNotAllowed_405];
							CreateHttpResponse(http_manager.GetResponse(), HttpVersion::Http_Version_1_1, HttpStatusCode::MethodNotAllowed_405, std::move(body));
						} else {
							std::string body = "Method NotAllowed";
							CreateHttpResponse(http_manager.GetResponse(), HttpVersion::Http_Version_1_1, HttpStatusCode::MethodNotAllowed_405, std::move(body));
						}
						wholder.Reset(http_manager.GetResponse()->EncodePdu());
						goto step2;
					}
				}
			}
		}

step2:
		while (true)
		{
			int length = co_await galay::AsyncSend(socket, &wholder, wholder->m_size);
			if( length <= 0 ) {
				close_connection = true;
				break;
			} else {
				if(wholder->m_offset >= wholder->m_size) {
					break;
				}
			}
		}
		if(http_manager.GetResponse()->Header()->HeaderPairs().GetValue("Connection") == "Close" || http_manager.GetResponse()->Header()->HeaderPairs().GetValue("Connection") == "close") {
			close_connection = true;
		}
		// clear
		rholder.ClearBuffer(), wholder.ClearBuffer();
		http_manager.GetRequest()->Reset(), http_manager.GetResponse()->Reset();
		if(close_connection) {
			bool res = co_await AsyncClose(socket);
			break;
		}
	}
	co_return;
}

void HttpServer::CreateHttpResponse(protocol::http::HttpResponse* response, protocol::http::HttpVersion version, protocol::http::HttpStatusCode code, std::string &&body)
{
	helper::http::HttpHelper::DefaultHttpResponse(response, version, code, "text/html", std::move(body));
}

util::ObjectPoolMutiThread<HttpsServer::HttpRequest> HttpsServer::RequestPool(DEFAULT_HTTP_REQUEST_POOL_SIZE);
util::ObjectPoolMutiThread<HttpsServer::HttpResponse> HttpsServer::ResponsePool(DEFAULT_HTTP_RESPONSE_POOL_SIZE);

HttpsServer::HttpsServer(const HttpsServerConfig::ptr &config)
	: m_server(config), m_store(std::make_unique<TcpSslCallbackStore>([this](const TcpSslConnectionManager& manager)->coroutine::Coroutine {
		return HttpRoute(manager);
	}))
{
}

void HttpsServer::Get(const std::string &path, std::function<Coroutine(HttpsConnectionManager, RoutineContext_ptr)> &&handler)
{
	m_route_map[protocol::http::HttpMethod::Http_Method_Get][path] = std::forward<std::function<coroutine::Coroutine(HttpsConnectionManager, RoutineContext_ptr)>>(handler);
}

void HttpsServer::Post(const std::string &path, std::function<Coroutine(HttpsConnectionManager, RoutineContext_ptr)> &&handler)
{
	m_route_map[protocol::http::HttpMethod::Http_Method_Post][path] = std::forward<std::function<coroutine::Coroutine(HttpsConnectionManager, RoutineContext_ptr)>>(handler);
}

void HttpsServer::Put(const std::string &path, std::function<Coroutine(HttpsConnectionManager, RoutineContext_ptr)> &&handler)
{
	m_route_map[protocol::http::HttpMethod::Http_Method_Put][path] = std::forward<std::function<coroutine::Coroutine(HttpsConnectionManager, RoutineContext_ptr)>>(handler);
}

void HttpsServer::Delete(const std::string &path, std::function<Coroutine(HttpsConnectionManager, RoutineContext_ptr)> &&handler)
{
	m_route_map[protocol::http::HttpMethod::Http_Method_Delete][path] = std::forward<std::function<coroutine::Coroutine(HttpsConnectionManager, RoutineContext_ptr)>>(handler);
}

void HttpsServer::Options(const std::string &path, std::function<Coroutine(HttpsConnectionManager, RoutineContext_ptr)> &&handler)
{
	m_route_map[protocol::http::HttpMethod::Http_Method_Options][path] = std::forward<std::function<coroutine::Coroutine(HttpsConnectionManager, RoutineContext_ptr)>>(handler);
}

void HttpsServer::Patch(const std::string &path, std::function<Coroutine(HttpsConnectionManager, RoutineContext_ptr)> &&handler)
{
	m_route_map[protocol::http::HttpMethod::Http_Method_Patch][path] = std::forward<std::function<coroutine::Coroutine(HttpsConnectionManager, RoutineContext_ptr)>>(handler);
}

void HttpsServer::Head(const std::string &path, std::function<Coroutine(HttpsConnectionManager, RoutineContext_ptr)> &&handler)
{
	m_route_map[protocol::http::HttpMethod::Http_Method_Head][path] = std::forward<std::function<coroutine::Coroutine(HttpsConnectionManager, RoutineContext_ptr)>>(handler);
}

void HttpsServer::Trace(const std::string &path, std::function<Coroutine(HttpsConnectionManager, RoutineContext_ptr)> &&handler)
{
	m_route_map[protocol::http::HttpMethod::Http_Method_Trace][path] = std::forward<std::function<coroutine::Coroutine(HttpsConnectionManager, RoutineContext_ptr)>>(handler);
}

void HttpsServer::Connect(const std::string &path, std::function<Coroutine(HttpsConnectionManager, RoutineContext_ptr)> &&handler)
{
	m_route_map[protocol::http::HttpMethod::Http_Method_Connect][path] = std::forward<std::function<coroutine::Coroutine(HttpsConnectionManager, RoutineContext_ptr)>>(handler);
}

void HttpsServer::Start(int port)
{
	m_server.Start(m_store.get(), port);
}

void HttpsServer::Stop()
{
	m_server.Stop();
}

coroutine::Coroutine HttpsServer::HttpRoute(TcpSslConnectionManager manager)
{
    co_await this_coroutine::AddToCoroutineStore();
	size_t max_header_size = std::dynamic_pointer_cast<HttpServerConfig>(m_server.GetConfig())->m_max_header_size;
	auto connection = manager.GetConnection();
	async::AsyncNetIo* socket = connection->GetSocket();
	IOVecHolder rholder(max_header_size), wholder;
	HttpsConnectionManager http_manager(manager, &RequestPool, &ResponsePool);
	auto [context, cancel] = coroutine::ContextFactory::WithWaitGroupContext();
	co_await this_coroutine::Exit([cancel, socket](){
		(*cancel)();
	});
	bool close_connection = false;
	while(true)
	{
step1:
		while(true) {
			int length = co_await AsyncRecv(socket, &rholder, max_header_size);
			if( length <= 0 ) {
				if( length == details::CommonFailedType::eCommonDisConnect || length == details::CommonFailedType::eCommonOtherFailed ) {
					bool res = co_await AsyncClose(socket);
					co_return;
				}
			} 
			else { 
				std::string_view data(rholder->m_buffer, rholder->m_offset);
				auto result = http_manager.GetRequest()->DecodePdu(data);
				if(!result.first) { //解析失败
					switch (http_manager.GetRequest()->GetErrorCode())
					{
					case error::HttpErrorCode::kHttpError_HeaderInComplete:
					case error::HttpErrorCode::kHttpError_BodyInComplete:
					{
						if( rholder->m_offset >= rholder->m_size) {
							rholder.Realloc(rholder->m_size * 2);
						}
						break;
					}
					case error::HttpErrorCode::kHttpError_BadRequest:
					{
						if(m_error_string.contains(HttpStatusCode::BadRequest_400)) {
							std::string body = m_error_string[HttpStatusCode::BadRequest_400];
							CreateHttpResponse(http_manager.GetResponse(), HttpVersion::Http_Version_1_1, HttpStatusCode::BadRequest_400, std::move(body));
						} else {
							std::string body = "Bad Request";
							CreateHttpResponse(http_manager.GetResponse(), HttpVersion::Http_Version_1_1, HttpStatusCode::BadRequest_400, std::move(body));
						}
						wholder.Reset(http_manager.GetResponse()->EncodePdu());
						close_connection = true;
						goto step2;
					}
					case error::HttpErrorCode::kHttpError_HeaderTooLong:
					{
						if(m_error_string.contains(HttpStatusCode::RequestHeaderFieldsTooLarge_431)) {
							std::string body = m_error_string[HttpStatusCode::RequestHeaderFieldsTooLarge_431];
							CreateHttpResponse(http_manager.GetResponse(), HttpVersion::Http_Version_1_1, HttpStatusCode::RequestHeaderFieldsTooLarge_431, std::move(body));
						} else {
							std::string body = "Header Too Long";
							CreateHttpResponse(http_manager.GetResponse(), HttpVersion::Http_Version_1_1, HttpStatusCode::RequestHeaderFieldsTooLarge_431, std::move(body));
						}
						wholder.Reset(http_manager.GetResponse()->EncodePdu());
						close_connection = true;
						goto step2;
					}
					case error::HttpErrorCode::kHttpError_UriTooLong:
					{
						if(m_error_string.contains(HttpStatusCode::UriTooLong_414)) {
							std::string body = m_error_string[HttpStatusCode::UriTooLong_414];
							CreateHttpResponse(http_manager.GetResponse(), HttpVersion::Http_Version_1_1, HttpStatusCode::UriTooLong_414, std::move(body));
						} else {
							std::string body = "Uri Too Long";
							CreateHttpResponse(http_manager.GetResponse(), HttpVersion::Http_Version_1_1, HttpStatusCode::UriTooLong_414, std::move(body));
						}
						wholder.Reset(http_manager.GetResponse()->EncodePdu());
						close_connection = true;
						goto step2;
					}
					default:
						break;
					}
				}
				else { //解析成功
					auto it = m_route_map.find(http_manager.GetRequest()->Header()->Method());
					if(it != m_route_map.end()) {
						auto it2 = it->second.find(http_manager.GetRequest()->Header()->Uri());
						if(it2 != it->second.end()) {
							it2->second(http_manager, context);
							wholder.Reset(http_manager.GetResponse()->EncodePdu());
							goto step2;
						} else { //没有该URI
					 		if(m_error_string.contains(HttpStatusCode::NotFound_404)) {
								std::string body = m_error_string[HttpStatusCode::NotFound_404];
								CreateHttpResponse(http_manager.GetResponse(), HttpVersion::Http_Version_1_1, HttpStatusCode::NotFound_404, std::move(body));
							} else {
								std::string body = "Not Found";
								CreateHttpResponse(http_manager.GetResponse(), HttpVersion::Http_Version_1_1, HttpStatusCode::NotFound_404, std::move(body));
							}
							wholder.Reset(http_manager.GetResponse()->EncodePdu());
							goto step2;
						}
					} else { //没有该方法
						if(m_error_string.contains(HttpStatusCode::MethodNotAllowed_405)) {
							std::string body = m_error_string[HttpStatusCode::MethodNotAllowed_405];
							CreateHttpResponse(http_manager.GetResponse(), HttpVersion::Http_Version_1_1, HttpStatusCode::MethodNotAllowed_405, std::move(body));
						} else {
							std::string body = "Method NotAllowed";
							CreateHttpResponse(http_manager.GetResponse(), HttpVersion::Http_Version_1_1, HttpStatusCode::MethodNotAllowed_405, std::move(body));
						}
						wholder.Reset(http_manager.GetResponse()->EncodePdu());
						goto step2;
					}
				}
			}
		}

step2:
		while (true)
		{
			int length = co_await AsyncSend(socket, &wholder, wholder->m_size);
			if( length <= 0 ) {
				close_connection = true;
				break;
			} else {
				if(wholder->m_offset >= wholder->m_size) {
					break;
				}
			}
		}
		if(http_manager.GetResponse()->Header()->HeaderPairs().GetValue("Connection") == "Close" || http_manager.GetResponse()->Header()->HeaderPairs().GetValue("Connection") == "close") {
			close_connection = true;
		}
		// clear
		rholder.ClearBuffer(), wholder.ClearBuffer();
		http_manager.GetRequest()->Reset(), http_manager.GetResponse()->Reset();
		if(close_connection) {
			bool res = co_await AsyncClose(socket);
			break;
		}
	}
	co_return;
}

void HttpsServer::CreateHttpResponse(HttpResponse *response, HttpVersion version, HttpStatusCode code, std::string &&body)
{
	helper::http::HttpHelper::DefaultHttpResponse(response, version, code, "text/html", std::move(body));
}
}