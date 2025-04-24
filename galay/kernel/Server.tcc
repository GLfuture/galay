#ifndef GALAY_SERVER_TCC
#define GALAY_SERVER_TCC

#include "Server.hpp"

namespace galay
{

template<typename Socket>
inline Connection<Socket>::Connection(std::unique_ptr<Socket> socket) 
    :m_socket(std::move(socket)) 
{
}

template <typename Socket>
inline std::pair<std::string, uint16_t> Connection<Socket>::GetRemoteAddr() const
{
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    char ip_str[INET6_ADDRSTRLEN];
    uint16_t port = 0;

    if (getpeername(m_socket->GetHandle().fd, (struct sockaddr*)&addr, &addr_len) == 0) {
        if (addr.ss_family == AF_INET) {
            struct sockaddr_in* s = (struct sockaddr_in*)&addr;
            inet_ntop(AF_INET, &s->sin_addr, ip_str, sizeof(ip_str));
            port = ntohs(s->sin_port);
        } else if (addr.ss_family == AF_INET6) {
            struct sockaddr_in6* s = (struct sockaddr_in6*)&addr;
            inet_ntop(AF_INET6, &s->sin6_addr, ip_str, sizeof(ip_str));
            port = ntohs(s->sin6_port);
        }
    }
    
    return {ip_str, port};
}


template <typename Socket>
inline std::unique_ptr<Socket> &Connection<Socket>::GetSocket()
{
    return m_socket;
}

template<typename Socket>
template <typename CoRtn>
AsyncResult<int, CoRtn> Connection<Socket>::Recv(TcpIOVecHolder& holder, int size, int64_t timeout_ms)
{
    return m_socket->template Recv<CoRtn>(holder, size, timeout_ms);
}

template <typename Socket>
template <typename CoRtn>
inline AsyncResult<int, CoRtn> Connection<Socket>::Send(TcpIOVecHolder& holder, int size, int64_t timeout_ms)
{
    return m_socket->template Send<CoRtn>(holder, size, timeout_ms);
}

template <typename Socket>
template <typename CoRtn>
inline AsyncResult<int, CoRtn> Connection<Socket>::SendFile(FileDesc* desc, int64_t timeout_ms)
{
    return m_socket->template SendFile<CoRtn>(desc, timeout_ms);
}

template <typename Socket>
template <typename CoRtn>
inline AsyncResult<bool, CoRtn> Connection<Socket>::Close()
{
    return m_socket->Close();
}

}

namespace galay::details
{

template <typename Socket>
CallbackStore<Socket>::callback_t CallbackStore<Socket>::m_callback = nullptr;

template <typename Socket>
inline void CallbackStore<Socket>::RegisteCallback(callback_t callback)
{
    m_callback = callback;
}

template <typename Socket>
inline void CallbackStore<Socket>::CreateConnAndExecCallback(EventScheduler* scheduler, std::unique_ptr<Socket> socket) 
{
    auto connection = std::make_unique<Connection<Socket>>(std::move(socket));
    if(m_callback) m_callback(RoutineCtx::Create(scheduler), std::move(connection));
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
    galay::details::CreateConnection(ctx, m_socket.get(), scheduler);
}

template <typename SocketType>
inline ListenEvent<SocketType>::~ListenEvent()
{
    if(m_scheduler) m_scheduler.load()->GetEngine()->DelEvent(this, nullptr);
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



namespace galay
{

template <typename SocketType>
inline void TcpServer<SocketType>::OnCall(callback_t callback)
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
}

#endif