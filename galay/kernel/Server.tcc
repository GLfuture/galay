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
    auto connection = std::make_shared<Connection<Socket>>(scheduler, socket);
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



namespace galay
{


template <typename SocketType>
inline void TcpServer<SocketType>::Prepare(callback_t callback)
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