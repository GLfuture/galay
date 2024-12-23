#ifndef GALAY_SERVER_TCC
#define GALAY_SERVER_TCC

#include "Server.hpp"

namespace galay::server {

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
inline Coroutine HttpServer<SocketType>::HttpRoute(std::shared_ptr<Connection<SocketType>> connection)
{
    co_await this_coroutine::AddToCoroutineStore();
    size_t max_header_size = std::dynamic_pointer_cast<HttpServerConfig>(m_server.GetConfig())->m_max_header_size;
    SocketType* socket = connection->GetSocket();
    IOVecHolder<TcpIOVec> rholder(max_header_size), wholder;
    Session session(connection, &RequestPool, &ResponsePool);
    auto [context, cancel] = ContextFactory::WithWaitGroupContext();
    co_await this_coroutine::DeferExit(std::bind([](RoutineContextCancel::ptr cancel){
        (*cancel)();
    }, cancel));
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
                    std::string res = HttpRouteHandler<SocketType>::GetInstance()->Handle(method, session.GetRequest()->Header()->Uri(), session, context);
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