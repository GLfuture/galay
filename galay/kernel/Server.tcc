#ifndef GALAY_SERVER_TCC
#define GALAY_SERVER_TCC

#include "Server.hpp"

namespace galay::details
{

template <typename Socket>
CallbackStore<Socket>::callback_t CallbackStore<Socket>::m_callback = nullptr;

template <typename Socket>
void CallbackStore<Socket>::RegisteCallback(callback_t callback)
{
    m_callback = callback;
}

template <typename Socket>
void CallbackStore<Socket>::CreateConnAndExecCallback(EventScheduler* scheduler, Socket* socket) 
{
    auto connection = std::make_shared<Connection<Socket>>(socket);
    if(m_callback) m_callback(RoutineCtx::Create(scheduler), connection);
}

template <typename SocketType>
inline ListenEvent<SocketType>::ListenEvent(EventScheduler* scheduler, SocketType* socket)
    : m_socket(socket), m_scheduler(scheduler)
{
    scheduler->GetEngine()->AddEvent(this, nullptr);
}


template <typename SocketType>
inline void ListenEvent<SocketType>::HandleEvent(EventEngine *engine)
{
    engine->ModEvent(this, nullptr);
    this->CreateConnection(RoutineCtx::Create(m_scheduler), m_scheduler);
}


template <typename SocketType>
inline std::string ListenEvent<SocketType>::Name() 
{ 
    return "UnkownEvent"; 
}

template <typename SocketType>
inline EventType ListenEvent<SocketType>::GetEventType() 
{ 
    return kEventTypeRead; 
}

template <typename SocketType>
inline GHandle ListenEvent<SocketType>::GetHandle()
{
    return m_socket->GetHandle();
}

template <typename SocketType>
inline bool ListenEvent<SocketType>::SetEventEngine(EventEngine* engine)
{
    return true;
}

template <typename SocketType>
inline EventEngine* ListenEvent<SocketType>::BelongEngine()
{
    return m_scheduler.load()->GetEngine();
}

template <typename SocketType>
inline void ListenEvent<SocketType>::CreateConnection(RoutineCtx ctx, EventScheduler* scheduler) {
    galay::details::CreateConnection(ctx.Copy(), m_socket, scheduler);
}

template <typename SocketType>
inline ListenEvent<SocketType>::~ListenEvent()
{
    if(m_scheduler) m_scheduler.load()->GetEngine()->DelEvent(this, nullptr);
    delete m_socket;
}

template<>
inline std::string ListenEvent<galay::AsyncTcpSocket>::Name()
{
    return "TcpListenEvent";
}

template<>
inline std::string ListenEvent<AsyncTcpSslSocket>::Name()
{
    return "TcpSslListenEvent";
}

}



namespace galay::server 
{

template <HttpStatusCode Code>
inline std::string CodeResponse<Code>::ResponseStr(HttpVersion version)
{
    if(m_responseStr.empty()) {
        m_responseStr = DefaultResponse(version);
    }
    return m_responseStr;
}

template <HttpStatusCode Code>
inline bool server::CodeResponse<Code>::RegisterResponse(HttpResponse response)
{
    static_assert(response.Header()->Code() == Code , "HttpStatusCode not match");
    m_responseStr = response.EncodePdu();
    return true;
}

template <HttpStatusCode Code>
inline std::string server::CodeResponse<Code>::DefaultResponse(HttpVersion version)
{
    HttpResponse response;
    response.Header()->Code() = Code;
    response.Header()->Version() = version;
    response.Header()->HeaderPairs().AddHeaderPair("Content-Type", "text/html");
    response.Header()->HeaderPairs().AddHeaderPair("Server", "galay");
    response.Header()->HeaderPairs().AddHeaderPair("Date", utils::GetCurrentGMTTimeString());
    response.Header()->HeaderPairs().AddHeaderPair("Connection", "close");
    response.SetContent("html", DefaultResponseBody());
    return response.EncodePdu();
}

template<>
inline std::string CodeResponse<HttpStatusCode::BadRequest_400>::DefaultResponseBody()
{
    return "<html>Bad Request</html>";
}


template<>
inline std::string CodeResponse<HttpStatusCode::NotFound_404>::DefaultResponseBody()
{
    return "<html>404 Not Found</html>";
}

template<>
inline std::string CodeResponse<HttpStatusCode::MethodNotAllowed_405>::DefaultResponseBody()
{
    return "<html>405 Method Not Allowed</html>";
}

template <>
inline std::string CodeResponse<HttpStatusCode::UriTooLong_414>::DefaultResponseBody()
{
    return "<html>Uri Too Long</html>";
}

template<>
inline std::string CodeResponse<HttpStatusCode::RequestTimeout_408>::DefaultResponseBody()
{
    return "<html>408 Request Timeout</html>";
}

template <>
inline std::string CodeResponse<HttpStatusCode::RequestHeaderFieldsTooLarge_431>::DefaultResponseBody()
{
    return "<html>Header Too Long</html>";
}

template<>
inline std::string CodeResponse<HttpStatusCode::InternalServerError_500>::DefaultResponseBody()
{
    return "<html>500 Internal Server Error</html>";
}

template <typename SocketType>
inline void server::TcpServer<SocketType>::Prepare(callback_t callback)
{
    details::CallbackStore<SocketType>::RegisteCallback(callback);
}

template <typename SocketType>
inline void TcpServer<SocketType>::Start(THost host)
{
    m_is_running = true;
    int requiredEventSchedulerNum = std::min(m_config->m_requiredEventSchedulerNum, EventSchedulerHolder::GetInstance()->GetSchedulerSize());
    if(requiredEventSchedulerNum == 0) {
        throw std::runtime_error("no scheduler available");
    }
    m_listen_events.resize(requiredEventSchedulerNum);
    for(int i = 0 ; i < requiredEventSchedulerNum; ++i )
    {
        SocketType* socket = new SocketType();
        if(const bool res = socket->Socket(); !res ) {
            delete socket;
            for(int j = 0; j < i; ++j ){
                delete m_listen_events[j];
            }
            m_listen_events.clear();
            LogError("[SocketType Init failed, error:{}]", error::GetErrorString(socket->GetErrorCode()));
            return;
        }
        HandleOption option(socket->GetHandle());
        option.HandleReuseAddr();
        option.HandleReusePort();
        
        if(!socket->Bind(host.m_ip, host.m_port)) {
            LogError("[Bind failed, error:{}]", error::GetErrorString(socket->GetErrorCode()));
            delete socket;
            for(int j = 0; j < i; ++j ){
                delete m_listen_events[j];
            }
            m_listen_events.clear();
            return;
        }
        if(!socket->Listen(m_config->m_backlog)) {
            LogError("[Listen failed, error:{}]", error::GetErrorString(socket->GetErrorCode()));
            delete socket;
            for(int j = 0; j < i; ++j ){
                delete m_listen_events[j];
            }
            m_listen_events.clear();
            return;
        }
        m_listen_events[i] = new details::ListenEvent<SocketType>(EventSchedulerHolder::GetInstance()->GetScheduler(i), socket);
    }
}

template<typename SocketType>
inline void TcpServer<SocketType>::Stop() 
{
    if(!m_is_running) return;
    for(const auto & m_listen_event : m_listen_events)
    {
        delete m_listen_event;
    }
    m_is_running = false;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

template <typename SocketType>
inline void server::HttpRouteHandler<SocketType>::AddHandler(HttpMethod method, const std::string &path, std::function<Coroutine<void>(RoutineCtx, SessionPtr)> &&handler)
{
    m_handler_map[method][path] = std::move(handler);
}

template <typename SocketType>
inline HttpRouteHandler<SocketType> *server::HttpRouteHandler<SocketType>::GetInstance()
{
    if(m_instance == nullptr) {
        m_instance = std::make_unique<HttpRouteHandler<SocketType>>();
    }
    return m_instance.get();
}

template <typename SocketType>
inline Coroutine<std::string> HttpRouteHandler<SocketType>::Handler(RoutineCtx ctx, HttpMethod method, const std::string &path, SessionPtr session)
{
    return Handle<SocketType>(ctx.Copy(), method, path, session, m_handler_map);
}

template <typename SocketType>
inline server::HttpServer<SocketType>::HttpServer(HttpServerConfig::ptr config)
    :m_server(config)
{
}

template <typename SocketType>
inline void server::HttpServer<SocketType>::Start(THost host)
{
    m_server.Prepare([this](RoutineCtx ctx, Connection<SocketType>::ptr connection) {
        typename Session<SocketType, HttpRequest, HttpResponse>::ptr session = std::make_shared<Session<SocketType, HttpRequest, HttpResponse>>(connection);
        return HttpRouteForward(ctx, session);
    });
    m_server.Start(host);
}

template <typename SocketType>
inline void server::HttpServer<SocketType>::Stop()
{
    m_server.Stop();
}

template <typename SocketType>
inline bool server::HttpServer<SocketType>::IsRunning() const
{
    return m_server.IsRunning();
}

template <typename SocketType>
template <HttpMethod... Methods>
inline void HttpServer<SocketType>::RouteHandler(const std::string &path, std::function<galay::Coroutine<void>(RoutineCtx,SessionPtr)> &&handler)
{
    ([&](){
        HttpRouteHandler<SocketType>::GetInstance()->AddHandler(Methods, path, std::move(handler));
    }(), ...);
}

template <typename SocketType>
inline Coroutine<void> HttpServer<SocketType>::HttpRouteForward(RoutineCtx ctx, typename Session<SocketType, HttpRequest, HttpResponse>::ptr session)
{
    return HttpRoute<SocketType>(ctx.Copy(), std::dynamic_pointer_cast<HttpServerConfig>(m_server.GetConfig())->m_max_header_size, session);
}

template <typename SocketType> 
inline void HttpServer<SocketType>::CreateHttpResponse(HttpResponse* response, HttpVersion version, HttpStatusCode code, std::string&& body)
{
    http::HttpHelper::DefaultHttpResponse(response, version, code, "text/html", std::move(body));
}


template<typename SocketType>
inline Coroutine<void> HttpRoute(RoutineCtx ctx, size_t max_header_size, typename Session<SocketType, http::HttpRequest, http::HttpResponse>::ptr session)
{
    auto connection = session->GetConnection();
    IOVecHolder<TcpIOVec> rholder(max_header_size), wholder;
    std::atomic_bool close_connection = false;
    while(true)
    {
step1:
        while(true) {
            int length = co_await connection->Recv(rholder, max_header_size, 5000);
            if( length <= 0 ) {
                if( length == details::CommonFailedType::eCommonDisConnect || length == details::CommonFailedType::eCommonOtherFailed ) {
                    bool res = co_await connection->Close();
                    co_return;
                } else if( length == details::CommonFailedType::eCommonTimeOutFailed ) {
                    wholder.Reset(CodeResponse<HttpStatusCode::RequestTimeout_408>::ResponseStr(HttpVersion::Http_Version_1_1));
                    co_await connection->Send(wholder, wholder->m_size, 5000);
                    co_await connection->Close();
                    co_return;
                }
            } 
            else { 
                std::string_view data(rholder->m_buffer, rholder->m_offset);
                auto result = session->GetRequest()->DecodePdu(data);
                if(!result.first) { //解析失败
                    switch (session->GetRequest()->GetErrorCode())
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
                        wholder.Reset(CodeResponse<HttpStatusCode::BadRequest_400>::ResponseStr(HttpVersion::Http_Version_1_1));
                        close_connection = true;
                        goto step2;
                    }
                    case galay::error::HttpErrorCode::kHttpError_HeaderTooLong:
                    {
                        wholder.Reset(CodeResponse<HttpStatusCode::RequestHeaderFieldsTooLarge_431>::ResponseStr(HttpVersion::Http_Version_1_1));
                        close_connection = true;
                        goto step2;
                    }
                    case galay::error::HttpErrorCode::kHttpError_UriTooLong:
                    {
                        wholder.Reset(CodeResponse<HttpStatusCode::UriTooLong_414>::ResponseStr(HttpVersion::Http_Version_1_1));
                        close_connection = true;
                        goto step2;
                    }
                    default:
                        break;
                    }
                }
                else { //解析成功
                    HttpMethod method = session->GetRequest()->Header()->Method();
                    auto co = co_await this_coroutine::WaitAsyncExecute<std::string, void>(\
                                HttpRouteHandler<SocketType>::GetInstance()->Handler(ctx, method,\
                                    session->GetRequest()->Header()->Uri(), session));
                    std::string resp = (*co)().value();
                    wholder.Reset(std::move(resp));
                    if(session->IsClose()) close_connection = true;
                    goto step2;
                }
            }
        }

step2:
        while (true)
        {
            int length = co_await connection->Send(wholder, wholder->m_size, 5000);
            if( length <= 0 ) {
                close_connection = true;
                break;
            } else {
                if(wholder->m_offset >= wholder->m_size) {
                    break;
                }
            }
        }
        // clear
        rholder.ClearBuffer(), wholder.ClearBuffer();
        session->GetRequest()->Reset(), session->GetResponse()->Reset();
        if(close_connection) {
            break;
        }
    }
    bool res = co_await connection->Close();
    co_return;
}


template <typename SocketType>
inline Coroutine<std::string> Handle(RoutineCtx ctx, http::HttpMethod method, const std::string &path, \
                                    typename galay::Session<SocketType, http::HttpRequest, http::HttpResponse>::ptr session, \
                                    typename HttpRouteHandler<SocketType>::HandlerMap& handlerMap)
{
    auto it = handlerMap.find(method);
    auto version = session->GetRequest()->Header()->Version();
    if(it == handlerMap.end()) {
        session->Close();
        co_return CodeResponse<HttpStatusCode::MethodNotAllowed_405>::ResponseStr(version);
    }
    auto uriit = it->second.find(path);
    if(uriit != it->second.end()) {
        http::HttpHelper::DefaultOK(session->GetResponse(), HttpVersion::Http_Version_1_1);
        co_await this_coroutine::WaitAsyncExecute<void, std::string>(uriit->second(ctx, session));
        if(session->IsClose()) {
            if(session->GetResponse()->Header()->HeaderPairs().HasKey("Connection")) {
                session->GetResponse()->Header()->HeaderPairs().SetHeaderPair("Connection", "close");
            } else {
                session->GetResponse()->Header()->HeaderPairs().AddHeaderPair("Connection", "close");
            }
        }
        co_return session->GetResponse()->EncodePdu();
    } else {
        session->Close();
        co_return CodeResponse<HttpStatusCode::NotFound_404>::ResponseStr(version);
    }
    session->Close();
    co_return CodeResponse<HttpStatusCode::InternalServerError_500>::ResponseStr(version);
}


}

#endif