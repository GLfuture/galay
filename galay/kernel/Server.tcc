#ifndef GALAY_SERVER_TCC
#define GALAY_SERVER_TCC

#include "Server.hpp"

namespace galay::server {

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
inline coroutine::Coroutine HttpServer<SocketType>::HttpRoute(std::shared_ptr<Connection<SocketType>> connection)
{
    co_await this_coroutine::AddToCoroutineStore();
    size_t max_header_size = std::dynamic_pointer_cast<HttpServerConfig>(m_server.GetConfig())->m_max_header_size;
    SocketType* socket = connection->GetSocket();
    IOVecHolder<TcpIOVec> rholder(max_header_size), wholder;
    Session session(connection, &RequestPool, &ResponsePool);
    auto [context, cancel] = coroutine::ContextFactory::WithWaitGroupContext();
    co_await this_coroutine::DeferExit(std::bind([](coroutine::RoutineContextCancel::ptr cancel){
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
                        if(m_error_string.contains(HttpStatusCode::BadRequest_400)) {
                            std::string body = m_error_string[HttpStatusCode::BadRequest_400];
                            CreateHttpResponse(session.GetResponse(), HttpVersion::Http_Version_1_1, HttpStatusCode::BadRequest_400, std::move(body));
                        } else {
                            std::string body = "Bad Request";
                            CreateHttpResponse(session.GetResponse(), HttpVersion::Http_Version_1_1, HttpStatusCode::BadRequest_400, std::move(body));
                        }
                        wholder.Reset(session.GetResponse()->EncodePdu());
                        close_connection = true;
                        goto step2;
                    }
                    case galay::error::HttpErrorCode::kHttpError_HeaderTooLong:
                    {
                        if(m_error_string.contains(HttpStatusCode::RequestHeaderFieldsTooLarge_431)) {
                            std::string body = m_error_string[HttpStatusCode::RequestHeaderFieldsTooLarge_431];
                            CreateHttpResponse(session.GetResponse(), HttpVersion::Http_Version_1_1, HttpStatusCode::RequestHeaderFieldsTooLarge_431, std::move(body));
                        } else {
                            std::string body = "Header Too Long";
                            CreateHttpResponse(session.GetResponse(), HttpVersion::Http_Version_1_1, HttpStatusCode::RequestHeaderFieldsTooLarge_431, std::move(body));
                        }
                        wholder.Reset(session.GetResponse()->EncodePdu());
                        close_connection = true;
                        goto step2;
                    }
                    case galay::error::HttpErrorCode::kHttpError_UriTooLong:
                    {
                        if(m_error_string.contains(HttpStatusCode::UriTooLong_414)) {
                            std::string body = m_error_string[HttpStatusCode::UriTooLong_414];
                            CreateHttpResponse(session.GetResponse(), HttpVersion::Http_Version_1_1, HttpStatusCode::UriTooLong_414, std::move(body));
                        } else {
                            std::string body = "Uri Too Long";
                            CreateHttpResponse(session.GetResponse(), HttpVersion::Http_Version_1_1, HttpStatusCode::UriTooLong_414, std::move(body));
                        }
                        wholder.Reset(session.GetResponse()->EncodePdu());
                        close_connection = true;
                        goto step2;
                    }
                    default:
                        break;
                    }
                }
                else { //解析成功
                    auto it = m_route_map.find(session.GetRequest()->Header()->Method());
                    if(it != m_route_map.end()) {
                        auto it2 = it->second.find(session.GetRequest()->Header()->Uri());
                        if(it2 != it->second.end()) {
                            it2->second(session, context);
                            wholder.Reset(session.GetResponse()->EncodePdu());
                            goto step2;
                        } else { //没有该URI
                            if(m_error_string.contains(HttpStatusCode::NotFound_404)) {
                                std::string body = m_error_string[HttpStatusCode::NotFound_404];
                                CreateHttpResponse(session.GetResponse(), HttpVersion::Http_Version_1_1, HttpStatusCode::NotFound_404, std::move(body));
                            } else {
                                std::string body = "Not Found";
                                CreateHttpResponse(session.GetResponse(), HttpVersion::Http_Version_1_1, HttpStatusCode::NotFound_404, std::move(body));
                            }
                            wholder.Reset(session.GetResponse()->EncodePdu());
                            goto step2;
                        }
                    } else { //没有该方法
                        if(m_error_string.contains(HttpStatusCode::MethodNotAllowed_405)) {
                            std::string body = m_error_string[HttpStatusCode::MethodNotAllowed_405];
                            CreateHttpResponse(session.GetResponse(), HttpVersion::Http_Version_1_1, HttpStatusCode::MethodNotAllowed_405, std::move(body));
                        } else {
                            std::string body = "Method NotAllowed";
                            CreateHttpResponse(session.GetResponse(), HttpVersion::Http_Version_1_1, HttpStatusCode::MethodNotAllowed_405, std::move(body));
                        }
                        wholder.Reset(session.GetResponse()->EncodePdu());
                        goto step2;
                    }
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