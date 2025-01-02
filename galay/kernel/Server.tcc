#ifndef GALAY_SERVER_TCC
#define GALAY_SERVER_TCC

#include "Server.hpp"

namespace galay::details
{


template <typename SocketType>
inline ListenEvent<SocketType>::ListenEvent(EventEngine* engine, SocketType* socket, CallbackStore<SocketType>* store)
    : m_socket(socket), m_store(store), m_engine(engine), m_scheduler(CoroutineSchedulerHolder::GetInstance()->GetScheduler())
{
    engine->AddEvent(this, nullptr);
}


template <typename SocketType>
inline void ListenEvent<SocketType>::HandleEvent(EventEngine *engine)
{
    engine->ModEvent(this, nullptr);
    this->CreateConnection(std::make_shared<RoutineCtx>(m_scheduler), engine);
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
    details::EventEngine* t = m_engine.load();
    if(!m_engine.compare_exchange_strong(t, engine)) {
        return false;
    }
    return true;
}

template <typename SocketType>
inline EventEngine* ListenEvent<SocketType>::BelongEngine()
{
    return m_engine.load();
}

template <typename SocketType>
inline void ListenEvent<SocketType>::CreateConnection(RoutineCtx::ptr ctx, EventEngine* engine) {
    galay::details::CreateConnection(ctx, m_socket, m_store, engine);
}

template <typename SocketType>
inline ListenEvent<SocketType>::~ListenEvent()
{
    if(m_engine) m_engine.load()->DelEvent(this, nullptr);
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


template<typename SocketType>
inline void TcpServer<SocketType>::Start(CallbackStore<SocketType>* store, THost host) {
    m_is_running = true;
    int requiredEventSchedulerNum = std::min(m_config->m_requiredEventSchedulerNum, EventSchedulerHolder::GetInstance()->GetSchedulerSize());
    if(requiredEventSchedulerNum == 0) {
        throw std::runtime_error("no scheduler available");
    }
    m_listen_events.resize(requiredEventSchedulerNum);
    for(int i = 0 ; i < requiredEventSchedulerNum; ++i )
    {
        SocketType* socket = new SocketType(EventSchedulerHolder::GetInstance()->GetScheduler(i)->GetEngine());
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
        m_listen_events[i] = new details::ListenEvent<SocketType>(EventSchedulerHolder::GetInstance()->GetScheduler(i)->GetEngine(), socket, store);
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
inline void server::HttpRouteHandler<SocketType>::AddHandler(HttpMethod method, const std::string &path, std::function<Coroutine<void>(RoutineCtx::ptr, Session)> &&handler)
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
inline Coroutine<std::string> HttpRouteHandler<SocketType>::Handler(RoutineCtx::ptr ctx, HttpMethod method, const std::string &path, galay::Session<SocketType, HttpRequest, HttpResponse> session)
{
    return Handle(ctx, method, path, session, m_handler_map);
}

template <typename SocketType>
inline server::HttpServer<SocketType>::HttpServer(HttpServerConfig::ptr config)
: m_server(config), 
        m_store(std::make_unique<CallbackStore<SocketType>>([this](RoutineCtx::ptr ctx,std::shared_ptr<Connection<SocketType>> connection)->Coroutine<void> {
                                                                    return HttpRouteForward(ctx, connection);
                                                                })) 
{

}

template <typename SocketType>
inline void server::HttpServer<SocketType>::Start(THost host)
{
    m_server.Start(m_store.get(), host);
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
inline void HttpServer<SocketType>::RouteHandler(const std::string &path, std::function<galay::Coroutine<void>(RoutineCtx::ptr, Session<SocketType, HttpRequest, HttpResponse>)> &&handler)
{
    ([&](){
        HttpRouteHandler<SocketType>::GetInstance()->AddHandler(Methods, path, std::move(handler));
    }(), ...);
}

template <typename SocketType>
inline Coroutine<void> HttpServer<SocketType>::HttpRouteForward(RoutineCtx::ptr ctx, std::shared_ptr<Connection<SocketType>> connection)
{
    return HttpRoute(ctx, std::dynamic_pointer_cast<HttpServerConfig>(m_server.GetConfig())->m_max_header_size, connection);
}

template <typename SocketType> 
inline void HttpServer<SocketType>::CreateHttpResponse(HttpResponse* response, HttpVersion version, HttpStatusCode code, std::string&& body)
{
    http::HttpHelper::DefaultHttpResponse(response, version, code, "text/html", std::move(body));
}







template<typename SocketType>
inline Coroutine<void> HttpRoute(RoutineCtx::ptr ctx, size_t max_header_size, std::shared_ptr<Connection<SocketType>> connection)
{
    //co_await this_coroutine::AddToCoroutineStore();
    SocketType* socket = connection->GetSocket();
    IOVecHolder<TcpIOVec> rholder(max_header_size), wholder;
    Session session(connection, &HttpServer<SocketType>::RequestPool, &HttpServer<SocketType>::ResponsePool);
    bool close_connection = false;
    while(true)
    {
step1:
        while(true) {
            int length = co_await socket->Recv(&rholder, max_header_size);
            if( length <= 0 ) {
                if( length == details::CommonFailedType::eCommonDisConnect || length == details::CommonFailedType::eCommonOtherFailed ) {
                    bool res = co_await socket->Close();
                    co_return;
                }
            } 
            else { 
                std::string_view data(rholder->m_buffer, rholder->m_offset);
                auto result = session.GetRequest()->DecodePdu(data);
                if(!result.first) { //解析失败
                    switch (session.GetRequest()->GetErrorCode())
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
                    HttpMethod method = session.GetRequest()->Header()->Method();
                    auto co = co_await this_coroutine::WaitAsyncExecute<std::string, void>(HttpRouteHandler<SocketType>::GetInstance()->Handler(ctx, method, session.GetRequest()->Header()->Uri(), session));
                    std::string resp = (*co)().value();
                    wholder.Reset(std::move(resp));
                    goto step2;
                }
            }
        }

step2:
        while (true)
        {
            int length = co_await socket->Send(&wholder, wholder->m_size);
            if( length <= 0 ) {
                close_connection = true;
                break;
            } else {
                if(wholder->m_offset >= wholder->m_size) {
                    break;
                }
            }
        }
        if(session.GetResponse()->Header()->HeaderPairs().GetValue("Connection") == "Close" || session.GetResponse()->Header()->HeaderPairs().GetValue("Connection") == "close") {
            close_connection = true;
        }
        // clear
        rholder.ClearBuffer(), wholder.ClearBuffer();
        session.GetRequest()->Reset(), session.GetResponse()->Reset();
        if(close_connection) {
            bool res = co_await socket->Close();
            break;
        }
    }
    co_return;
}


template <typename SocketType>
inline Coroutine<std::string> Handle(RoutineCtx::ptr ctx, http::HttpMethod method, const std::string &path, \
                                    galay::Session<SocketType, http::HttpRequest, http::HttpResponse> session, \
                                    typename HttpRouteHandler<SocketType>::HandlerMap& handlerMap)
{
    auto it = handlerMap.find(method);
    auto version = session.GetRequest()->Header()->Version();
    if(it == handlerMap.end()) {
        co_return CodeResponse<HttpStatusCode::MethodNotAllowed_405>::ResponseStr(version);
    }
    auto uriit = it->second.find(path);
    if(uriit != it->second.end()) {
        http::HttpHelper::DefaultOK(session.GetResponse(), HttpVersion::Http_Version_1_1);
        co_await this_coroutine::WaitAsyncExecute<void, std::string>(uriit->second(ctx, session));
        if(session.IsClose()) {
            if(session.GetResponse()->Header()->HeaderPairs().HasKey("Connection")) {
                session.GetResponse()->Header()->HeaderPairs().SetHeaderPair("Connection", "close");
            } else {
                session.GetResponse()->Header()->HeaderPairs().AddHeaderPair("Connection", "close");
            }
        }
        co_return session.GetResponse()->EncodePdu();
    } else {
        co_return CodeResponse<HttpStatusCode::NotFound_404>::ResponseStr(version);
    }
    co_return CodeResponse<HttpStatusCode::InternalServerError_500>::ResponseStr(version);
}


}

#endif