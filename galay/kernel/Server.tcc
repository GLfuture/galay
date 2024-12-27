#ifndef GALAY_SERVER_TCC
#define GALAY_SERVER_TCC

#include "Server.hpp"

namespace galay::details
{
template <typename SocketType>
inline ListenEvent<SocketType>::ListenEvent(EventEngine* engine, SocketType* socket, CallbackStore<SocketType>* store)
    : m_socket(socket), m_store(store), m_engine(engine)
{
    engine->AddEvent(this, nullptr);
}


template <typename SocketType>
inline void ListenEvent<SocketType>::HandleEvent(EventEngine *engine)
{
    engine->ModEvent(this, nullptr);
    CreateConnection(engine);
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
inline Coroutine<void> ListenEvent<SocketType>::CreateConnection(EventEngine* engine) {
    LogError("[not support [SocketType]]");
    co_return;
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

template<>
inline Coroutine<void> ListenEvent<galay::AsyncTcpSocket>::CreateConnection(EventEngine* engine)
{
    NetAddr addr{};
    while(true)
    {
        const GHandle handle = co_await m_socket->Accept(&addr);
        if( handle.fd == -1 ){
            if(const uint32_t error = m_socket->GetErrorCode(); error != error::Error_NoError ) {
                LogError("[{}]", error::GetErrorString(error));
            }
            co_return;
        }
        auto socket = new galay::AsyncTcpSocket(engine);
        if( !socket->Socket(handle) ) {
            delete socket;
            co_return;
        }
        LogTrace("[Handle:{}, Acceot Success]", socket->GetHandle().fd);
        engine->ResetMaxEventSize(handle.fd);
        this->m_store->Execute(socket);
    }
    co_return;
}

template<>
inline Coroutine<void> ListenEvent<AsyncTcpSslSocket>::CreateConnection(EventEngine* engine)
{
    NetAddr addr{};
    while (true)
    {
        const auto handle = co_await m_socket->Accept(&addr);
        if( handle.fd == -1 ){
            if(const uint32_t error = m_socket->GetErrorCode(); error != error::Error_NoError ) {
                LogError("[{}]", error::GetErrorString(error));
            }
            co_return;
        }
        auto socket = new AsyncTcpSslSocket(engine);
        if( bool res = socket->Socket(handle); !res ) {
            delete socket;
            co_return;
        }
        LogTrace("[Handle:{}, Accept Success]", socket->GetHandle().fd);
        if(const bool success = co_await socket->SSLAccept(); !success ){
            LogError("[{}]", error::GetErrorString(socket->GetErrorCode()));
            close(handle.fd);
            delete socket;
            co_return;
        }
        LogTrace("[Handle:{}, SSL_Acceot Success]", socket->GetHandle().fd);
        engine->ResetMaxEventSize(handle.fd);
        this->m_store->Execute(socket);
    }
    
}


}



namespace galay::server 
{

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
inline void TcpServer<SocketType>::Start(CallbackStore<SocketType>* store, const std::string& addr, int port) {
    m_is_running = true;
    m_listen_events.resize(m_config->m_netSchedulerConf.first);
    for(int i = 0 ; i < m_config->m_netSchedulerConf.first; ++i )
    {
        SocketType* socket = new SocketType(EeventSchedulerHolder::GetInstance()->GetScheduler(i)->GetEngine());
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
        
        if(!socket->Bind(addr, port)) {
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
        m_listen_events[i] = new details::ListenEvent<SocketType>(EeventSchedulerHolder::GetInstance()->GetScheduler(i)->GetEngine(), socket, store);
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


template<typename SocketType>
inline Coroutine<void> HttpServer<SocketType>::HttpRoute(std::shared_ptr<Connection<SocketType>> connection)
{
    //co_await this_coroutine::AddToCoroutineStore();
    size_t max_header_size = std::dynamic_pointer_cast<HttpServerConfig>(m_server.GetConfig())->m_max_header_size;
    SocketType* socket = connection->GetSocket();
    IOVecHolder<TcpIOVec> rholder(max_header_size), wholder;
    Session session(connection, &RequestPool, &ResponsePool);
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
                    std::string res = HttpRouteHandler<SocketType>::GetInstance()->Handle(method, session.GetRequest()->Header()->Uri(), session);
                    wholder.Reset(std::move(res));
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

}

#endif