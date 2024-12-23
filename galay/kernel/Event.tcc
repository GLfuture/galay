#ifndef GALAY_EVENT_TCC
#define GALAY_EVENT_TCC

#include "Event.hpp"

namespace galay::details
{

template <typename SocketType>
inline ListenEvent<SocketType>::ListenEvent(EventEngine* engine, SocketType* socket, CallbackStore<SocketType>* store)
    : m_socket(socket), m_store(store), m_engine(engine)
{
    engine->AddEvent(this, nullptr);
}

template <typename SocketType>
inline void ListenEvent<SocketType>::HandleEvent(EventEngine* engine)
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
inline Coroutine ListenEvent<SocketType>::CreateConnection(EventEngine* engine) {
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
inline Coroutine ListenEvent<galay::AsyncTcpSocket>::CreateConnection(EventEngine* engine)
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
        galay::AsyncTcpSocket* socket = new galay::AsyncTcpSocket(engine);
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
inline Coroutine ListenEvent<AsyncTcpSslSocket>::CreateConnection(EventEngine* engine)
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
        AsyncTcpSslSocket* socket = new AsyncTcpSslSocket(engine);
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

#endif