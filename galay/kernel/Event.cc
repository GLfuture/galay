#include "Event.h"
#include "Async.h"
#include "EventEngine.h"
#include "Scheduler.h"
#include "Connection.h"
#include "Time.h"
#include "ExternApi.h"
#include <cstring>
#if defined(__linux__)
#include <sys/timerfd.h>
#elif  defined(WIN32) || defined(_WIN32) || defined(_WIN32_) || defined(WIN64) || defined(_WIN64) || defined(_WIN64_)

#endif
#include "Log.h"

namespace galay::event
{

std::string ToString(const EventType type)
{
    switch (type)
    {
    case kEventTypeNone:
        return "EventTypeNone";
    case kEventTypeRead:
        return "EventTypeRead";
    case kEventTypeWrite:
        return "EventTypeWrite";
    case kEventTypeTimer:
        return "EventTypeTimer";
    case kEventTypeError:
        return "EventTypeError";
    default:
        break;
    }
    return ""; 
}

    
CallbackEvent::CallbackEvent(const GHandle handle, const EventType type, std::function<void(Event*, EventEngine*)> callback)
    : m_type(type), m_handle(handle), m_engine(nullptr), m_callback(std::move(callback))
{
    
}

void CallbackEvent::HandleEvent(EventEngine *engine)
{
    this->m_callback(this, engine);
}

bool CallbackEvent::SetEventEngine(EventEngine *engine)
{
    if(event::EventEngine* t = m_engine.load(); !m_engine.compare_exchange_strong(t, engine)) {
        return false;
    }
    return true;
}

EventEngine* CallbackEvent::BelongEngine()
{
    return m_engine.load();
}


CallbackEvent::~CallbackEvent()
{
    if( m_engine ) m_engine.load()->DelEvent(this, nullptr);
}

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
TimeEventIDStore::TimeEventIDStore(const int capacity)
{
    m_temp = static_cast<int*>(calloc(capacity, sizeof(int)));
    for(int i = 0; i < capacity; i++){
        m_temp[i] = i;
    }
    m_capacity = capacity;
    m_eventIds.enqueue_bulk(m_temp, capacity);
    free(m_temp);
}

bool TimeEventIDStore::GetEventId(int& id)
{
    return m_eventIds.try_dequeue(id);
}

bool TimeEventIDStore::RecycleEventId(const int id)
{
    return m_eventIds.enqueue(id);
}


TimeEventIDStore TimeEvent::g_idStore(DEFAULT_TIMEEVENT_ID_CAPACITY);

bool TimeEvent::CreateHandle(GHandle &handle)
{
    return g_idStore.GetEventId(handle.fd);
}


#elif defined(__linux__)

bool TimeEvent::CreateHandle(GHandle& handle)
{
    handle.fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    return handle.fd != -1;
}

#endif


TimeEvent::TimeEvent(const GHandle handle, EventEngine* engine)
    : m_handle(handle), m_engine(engine)
{
#if defined(__linux__)
    m_engine.load()->AddEvent(this, nullptr);
#endif
}

void TimeEvent::HandleEvent(EventEngine *engine)
{
#if defined(__linux__)
    std::vector<galay::Timer::ptr> timers;
    std::unique_lock lock(this->m_mutex);
    while (! m_timers.empty() && m_timers.top()->GetExpiredTime()  <= GetCurrentTime() ) {
        auto timer = m_timers.top();
        m_timers.pop();
        timers.emplace_back(timer);
    }
    lock.unlock();
    for (auto timer : timers)
    {
        timer->Execute();
    }
    UpdateTimers();
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    std::unique_lock lock(this->m_mutex);
    Timer::ptr timer = m_timers.top();
    m_timers.pop();
    lock.unlock();
    timer->Execute();
#endif
}

bool TimeEvent::SetEventEngine(EventEngine *engine)
{
    event::EventEngine* t = m_engine.load();
    if(!m_engine.compare_exchange_strong(t, engine)) {
        return false;
    }
    return true;
}

EventEngine *TimeEvent::BelongEngine()
{
    return m_engine;
}

galay::Timer::ptr TimeEvent::AddTimer(int64_t during_time, std::function<void(galay::Timer::ptr)> &&func)
{
    auto timer = std::make_shared<galay::Timer>(during_time, std::forward<std::function<void(galay::Timer::ptr)>>(func));
    std::unique_lock<std::shared_mutex> lock(this->m_mutex);
    this->m_timers.push(timer);
    lock.unlock();
#if defined(__linux__)
    UpdateTimers();
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    m_engine.load()->ModEvent(this, timer.get());
#endif
    return timer;
}

void TimeEvent::ReAddTimer(const int64_t during_time, const galay::Timer::ptr& timer)
{
    timer->SetDuringTime(during_time);
    timer->m_cancle.store(false);
    std::unique_lock lock(this->m_mutex);
    this->m_timers.push(timer);
    lock.unlock();
#if defined(__linux__)
    UpdateTimers();
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    m_engine.load()->ModEvent(this, timer.get());
#endif
}

TimeEvent::~TimeEvent()
{
    m_engine.load()->DelEvent(this, nullptr);
#if defined(__linux__)
    close(m_handle.fd);
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    g_idStore.RecycleEventId(m_handle.fd);
#endif
}

#if defined(__linux__)
void TimeEvent::UpdateTimers()
{
    struct timespec abstime;
    std::shared_lock lock(this->m_mutex);
    if (m_timers.empty())
    {
        abstime.tv_sec = 0;
        abstime.tv_nsec = 0;
    }
    else
    {
        auto timer = m_timers.top();
        lock.unlock();
        int64_t time = timer->GetRemainTime();
        if (time != 0)
        {
            abstime.tv_sec = time / 1000;
            abstime.tv_nsec = (time % 1000) * 1000000;
        }
        else
        {
            abstime.tv_sec = 0;
            abstime.tv_nsec = 1;
        }
    }
    struct itimerspec its = {
        .it_interval = {},
        .it_value = abstime};
    timerfd_settime(this->m_handle.fd, 0, &its, nullptr);
}
#endif

//#if defined(USE_EPOLL) && !defined(USE_AIO)

ListenEvent::ListenEvent(EventEngine* engine, async::AsyncNetIo* socket, TcpCallbackStore* callback_store)
    : m_socket(socket), m_callback_store(callback_store), m_engine(engine)
{
    engine->AddEvent(this, nullptr);
}

void ListenEvent::HandleEvent(EventEngine *engine)
{
    engine->ModEvent(this, nullptr);
    CreateTcpSocket(engine);
}

GHandle ListenEvent::GetHandle()
{
    return m_socket->GetHandle();
}

bool ListenEvent::SetEventEngine(EventEngine *engine)
{
    event::EventEngine* t = m_engine.load();
    if(!m_engine.compare_exchange_strong(t, engine)) {
        return false;
    }
    return true;
}

EventEngine* ListenEvent::BelongEngine()
{
    return m_engine.load();
}

ListenEvent::~ListenEvent()
{
    if(m_engine) m_engine.load()->DelEvent(this, nullptr);
    delete m_socket;
}

coroutine::Coroutine ListenEvent::CreateTcpSocket(EventEngine *engine)
{
    NetAddr addr;
    while(true)
    {
        const GHandle new_handle = co_await AsyncAccept(m_socket, &addr);
        if( new_handle.fd == -1 ){
            if(const uint32_t error = m_socket->GetErrorCode(); error != error::Error_NoError ) {
                LogError("[{}]", error::GetErrorString(error));
            }
            co_return;
        }
        async::AsyncNetIo* socket = new async::AsyncTcpSocket(engine);
        socket->GetHandle() = new_handle;
        LogTrace("[Handle:{}, Acceot Success]", socket->GetHandle().fd);
        auto option = socket->GetOption();
        if (!option.HandleNonBlock())
        {
            LogError("[{}]", error::GetErrorString(option.GetErrorCode()));
            close(new_handle.fd);
            delete socket;
            co_return;
        }
        engine->ResetMaxEventSize(new_handle.fd);
        this->m_callback_store->Execute(socket);
    }
    co_return;
}

SslListenEvent::SslListenEvent(EventEngine* engine, async::AsyncSslNetIo *socket, TcpSslCallbackStore *callback_store)
    :m_socket(socket), m_callback_store(callback_store), m_engine(engine), m_action(new action::IOEventAction(engine, new NetSslWaitEvent(socket)))
{
    engine->AddEvent(this, nullptr);
}

void SslListenEvent::HandleEvent(EventEngine *engine)
{
    engine->ModEvent(this, nullptr);
    CreateTcpSslSocket(engine);
}

GHandle SslListenEvent::GetHandle()
{
    return m_socket->GetHandle();
}

bool SslListenEvent::SetEventEngine(EventEngine *engine)
{
    event::EventEngine* t = m_engine.load();
    if(!m_engine.compare_exchange_strong(t, engine)) {
        return false;
    }
    return true;
}

EventEngine* SslListenEvent::BelongEngine()
{
    return m_engine.load();
}

SslListenEvent::~SslListenEvent()
{
    m_engine.load()->DelEvent(this, nullptr);
    delete m_action;
}

coroutine::Coroutine SslListenEvent::CreateTcpSslSocket(EventEngine *engine)
{
    NetAddr addr;
    while (true)
    {
        const auto [fd] = co_await AsyncAccept(m_socket, &addr);
        if( fd == -1 ){
            if(const uint32_t error = m_socket->GetErrorCode(); error != error::Error_NoError ) {
                LogError("[{}]", error::GetErrorString(error));
            }
            co_return;
        }
        async::AsyncSslNetIo* socket = new async::AsyncSslTcpSocket(engine);
        bool res = AsyncSSLSocket(socket, GetGlobalSSLCtx());
        if( !res ) {
            delete socket;
            continue;
        }
        LogTrace("[Handle:{}, Accept Success]", socket->GetHandle().fd);
        auto option = socket->GetOption();
        if (!option.HandleNonBlock())
        {
            LogError("[{}]", error::GetErrorString(option.GetErrorCode()));
            close(fd);
            delete socket;
            co_return;
        }
        if(const bool success = co_await AsyncSSLAccept(socket); !success ){
            LogError("[{}]", error::GetErrorString(socket->GetErrorCode()));
            close(fd);
            delete socket;
            co_return;
        }
        LogTrace("[Handle:{}, SSL_Acceot Success]", socket->GetHandle().fd);
        engine->ResetMaxEventSize(fd);
        this->m_callback_store->Execute(socket);
    }
    
}

WaitEvent::WaitEvent()
    :m_engine(nullptr), m_waitco(nullptr)
{
}

bool WaitEvent::SetEventEngine(EventEngine *engine)
{
    if(event::EventEngine* t = m_engine.load(); !m_engine.compare_exchange_strong(t, engine)) {
        return false;
    }
    return true;
}

EventEngine* WaitEvent::BelongEngine()
{
    return m_engine.load();
}


NetWaitEvent::NetWaitEvent(async::AsyncNetIo *socket)
    :m_type(kTcpWaitEventTypeError), m_socket(socket)
{
}

std::string NetWaitEvent::Name()
{
    switch (m_type)
    {
    case kTcpWaitEventTypeError:
        return "ErrorEvent";
    case kTcpWaitEventTypeAccept:
        return "AcceptEvent";
    case kTcpWaitEventTypeRecv:
        return "RecvEvent";
    case kTcpWaitEventTypeSend:
        return "SendEvent";
    case kTcpWaitEventTypeConnect:
        return "ConnectEvent";
    case kTcpWaitEventTypeClose:
        return "CloseEvent";
    default:
        break;
    }
    return "UnknownEvent";
}

bool NetWaitEvent::OnWaitPrepare(coroutine::Coroutine *co, void* ctx)
{
    this->m_waitco = co;
    this->m_ctx = ctx;
    switch (m_type)
    {
    case kTcpWaitEventTypeError:
        return false;
    case kTcpWaitEventTypeAccept:
        return OnAcceptWaitPrepare(co, ctx);
    case kTcpWaitEventTypeRecv:
        return OnRecvWaitPrepare(co, ctx);
    case kTcpWaitEventTypeSend:
        return OnSendWaitPrepare(co, ctx);
    case kTcpWaitEventTypeConnect:
        return OnConnectWaitPrepare(co, ctx);
    case kTcpWaitEventTypeClose:
        return OnCloseWaitPrepare(co, ctx);
    default:
        break;
    }
    return false;
}

void NetWaitEvent::HandleEvent(EventEngine *engine)
{
    switch (m_type)
    {
    case kTcpWaitEventTypeError:
        HandleErrorEvent(engine);
        return;
    case kTcpWaitEventTypeAccept:
        HandleAcceptEvent(engine);
        return;
    case kTcpWaitEventTypeRecv:
        HandleRecvEvent(engine);
        return;
    case kTcpWaitEventTypeSend:
        HandleSendEvent(engine);
        return;
    case kTcpWaitEventTypeConnect:
        HandleConnectEvent(engine);
        return;
    case kTcpWaitEventTypeClose:
        HandleCloseEvent(engine);
    default: ;
    }
}

EventType NetWaitEvent::GetEventType()
{
    switch (m_type)
    {
    case kTcpWaitEventTypeError:
        return kEventTypeError;
    case kTcpWaitEventTypeAccept:
    case kTcpWaitEventTypeRecv:
        return kEventTypeRead;
    case kTcpWaitEventTypeSend:
    case kTcpWaitEventTypeConnect:
        return kEventTypeWrite;
    case kTcpWaitEventTypeClose:
        return kEventTypeNone;
    default:
        break;
    }
    return kEventTypeNone;
}

GHandle NetWaitEvent::GetHandle()
{
    return m_socket->GetHandle();
}

NetWaitEvent::~NetWaitEvent()
{
    if(m_engine) m_engine.load()->DelEvent(this, nullptr);
}

bool NetWaitEvent::OnAcceptWaitPrepare(const coroutine::Coroutine *co, void *ctx) const
{
    NetAddr* netaddr = static_cast<NetAddr*>(ctx);
    sockaddr addr{};
    socklen_t addrlen = sizeof(addr);
    GHandle handle {
        .fd = accept(m_socket->GetHandle().fd, &addr, &addrlen),
    };
    netaddr->m_ip = inet_ntoa(((sockaddr_in*)&addr)->sin_addr);
    netaddr->m_port = ntohs(((sockaddr_in*)&addr)->sin_port);
    LogTrace("[Accept Address: {}:{}]", netaddr->m_ip, netaddr->m_port);
    galay::coroutine::Awaiter* awaiter = co->GetAwaiter();
    if( handle.fd < 0 ) {
        if( errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR ) {
            
        }else{
            m_socket->GetErrorCode() = error::MakeErrorCode(error::Error_AcceptError, errno);
        }
    }
    awaiter->SetResult(handle);
    return false;
}

bool NetWaitEvent::OnRecvWaitPrepare(const coroutine::Coroutine *co, void* ctx)
{
    auto* iov = static_cast<IOVec*>(ctx);
    int recvBytes = DealRecv(iov);
    galay::coroutine::Awaiter* awaiter = co->GetAwaiter();
    if( recvBytes == eCommonNonBlocking ) {
        return true;
    }
    awaiter->SetResult(recvBytes);
    return false;
}

bool NetWaitEvent::OnSendWaitPrepare(const coroutine::Coroutine *co, void* ctx)
{
    auto* iov = static_cast<IOVec*>(ctx);
    int sendBytes = DealSend(iov);
    galay::coroutine::Awaiter* awaiter = co->GetAwaiter();
    if( sendBytes == eCommonNonBlocking){
        return true;
    }
    awaiter->SetResult(sendBytes);
    return false;
}

bool NetWaitEvent::OnConnectWaitPrepare(coroutine::Coroutine *co, void* ctx) const
{
    auto* addr = static_cast<NetAddr*>(ctx);
    LogTrace("[Connect to {}:{}]", addr->m_ip, addr->m_port);
    sockaddr_in saddr{};
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = inet_addr(addr->m_ip.c_str());
    saddr.sin_port = htons(addr->m_port);
    const int ret = connect(m_socket->GetHandle().fd, reinterpret_cast<sockaddr*>(&saddr), sizeof(sockaddr_in));
    if( ret != 0) {
        if( errno == EWOULDBLOCK || errno == EINTR || errno == EAGAIN || errno == EINPROGRESS) {
            return true;
        }
    }
    galay::coroutine::Awaiter* awaiter = m_waitco->GetAwaiter();
    if( ret == 0 ){
        awaiter->SetResult(true);
    } else {
        m_socket->GetErrorCode() = error::MakeErrorCode(error::Error_ConnectError, errno);
        awaiter->SetResult(false);
    }
    return false;
}

bool NetWaitEvent::OnCloseWaitPrepare(const coroutine::Coroutine *co, void* ctx)
{
    coroutine::Awaiter* awaiter = co->GetAwaiter(); 
    if (m_engine) {
        if( m_engine.load()->DelEvent(this, nullptr) != 0 ) {
            awaiter->SetResult(false);
            m_socket->GetErrorCode() = error::MakeErrorCode(error::Error_CloseError, errno);
            return false;
        }
    }
    LogTrace("[Close {}]", m_socket->GetHandle().fd);
    if(const int ret = close(m_socket->GetHandle().fd); ret < 0 ) {
        m_socket->GetErrorCode() = error::MakeErrorCode(error::Error_CloseError, errno);
        awaiter->SetResult(false);
    } else {
        awaiter->SetResult(true);
    }
    return false;
}

void NetWaitEvent::HandleErrorEvent(EventEngine *engine)
{
}

void NetWaitEvent::HandleAcceptEvent(EventEngine *engine)
{
}

void NetWaitEvent::HandleRecvEvent(EventEngine *engine)
{
    int recvBytes = DealRecv(static_cast<IOVec*>(m_ctx));
    galay::coroutine::Awaiter* awaiter = m_waitco->GetAwaiter();
    awaiter->SetResult(recvBytes);
    //2.唤醒协程
    GetCoroutineScheduler(m_socket->GetHandle().fd % GetCoroutineSchedulerNum())->EnqueueCoroutine(m_waitco);
}

void NetWaitEvent::HandleSendEvent(EventEngine *engine)
{
    int sendBytes = DealSend(static_cast<IOVec*>(m_ctx));
    galay::coroutine::Awaiter* awaiter = m_waitco->GetAwaiter();
    awaiter->SetResult(sendBytes);
    GetCoroutineScheduler(m_socket->GetHandle().fd % GetCoroutineSchedulerNum())->EnqueueCoroutine(m_waitco);
}

void NetWaitEvent::HandleConnectEvent(EventEngine *engine) const
{
    galay::coroutine::Awaiter* awaiter = m_waitco->GetAwaiter();
    awaiter->SetResult(true);
    GetCoroutineScheduler(m_socket->GetHandle().fd % GetCoroutineSchedulerNum())->EnqueueCoroutine(m_waitco);
}

void NetWaitEvent::HandleCloseEvent(EventEngine *engine)
{ 
    //doing nothing
}

int NetWaitEvent::DealRecv(IOVec* vec)
{
    auto [fd] = this->m_socket->GetHandle();
    size_t recvBytes = vec->m_offset + vec->m_length > vec->m_size ? vec->m_size - vec->m_offset : vec->m_length;
    int length = recv(fd, vec->m_buffer + vec->m_offset, recvBytes, 0);
    if( length == -1 ) {
        if( errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR ) {
            m_socket->GetErrorCode() = error::MakeErrorCode(error::Error_RecvError, errno);
            return eCommonOtherFailed;
        } else {
            return eCommonNonBlocking;
        }
    }
    LogTrace("[Recv, Handle: {}, Byte: {}\nData: {}]", fd, length, std::string_view(vec->m_buffer + vec->m_offset, length));
    vec->m_offset += length;
    return length;
}

int NetWaitEvent::DealSend(IOVec* vec)
{
    auto [fd] = this->m_socket->GetHandle();
    size_t writeBytes = vec->m_offset + vec->m_length > vec->m_size ? vec->m_size - vec->m_offset : vec->m_length;
    const int length = send(fd, vec->m_buffer + vec->m_offset, writeBytes, 0);
    if( length == -1 ) {
        if( errno != EAGAIN && errno != EWOULDBLOCK && errno == EINTR ) {
            m_socket->GetErrorCode() = error::MakeErrorCode(error::Error_SendError, errno);
            return eCommonOtherFailed;
        } else {
            return eCommonNonBlocking;
        }
    }
    LogTrace("[Send, Handle: {}, Byte: {}\nData: {}]", fd, length, std::string_view(vec->m_buffer + vec->m_offset, length));
    vec->m_offset += length;
    return length;
}

NetSslWaitEvent::NetSslWaitEvent(async::AsyncSslNetIo *socket)
    :NetWaitEvent(socket)
{
}

std::string NetSslWaitEvent::Name()
{
    switch (m_type)
    {
    case kTcpWaitEventTypeError:
        return "ErrorEvent";
    case kTcpWaitEventTypeAccept:
        return "AcceptEvent";
    case kTcpWaitEventTypeSslAccept:
        return "SslAcceptEvent";
    case kTcpWaitEventTypeRecv:
        return "RecvEvent";
    case kTcpWaitEventTypeSslRecv:
        return "SslRecvEvent";
    case kTcpWaitEventTypeSend:
        return "SendEvent";
    case kTcpWaitEventTypeSslSend:
        return "SslSendEvent";
    case kTcpWaitEventTypeConnect:
        return "ConnectEvent";
    case kTcpWaitEventTypeSslConnect:
        return "SslConnectEvent";
    case kTcpWaitEventTypeClose:
        return "CloseEvent";
    case kTcpWaitEventTypeSslClose:
        return "SslCloseEvent";
    default:
        break;
    }
    return "UnknownEvent";
}

EventType NetSslWaitEvent::GetEventType()
{
    switch (m_type)
    {
    case kTcpWaitEventTypeError:
        return kEventTypeError;
    case kTcpWaitEventTypeAccept:
        return kEventTypeRead;
    case kTcpWaitEventTypeSslAccept:
        return CovertSSLErrorToEventType();
    case kTcpWaitEventTypeRecv:
    case kTcpWaitEventTypeSslRecv:
        return kEventTypeRead;
    case kTcpWaitEventTypeSend:
    case kTcpWaitEventTypeSslSend:
    case kTcpWaitEventTypeConnect:
        return kEventTypeWrite;
    case kTcpWaitEventTypeSslConnect:
        return CovertSSLErrorToEventType();
    case kTcpWaitEventTypeClose:
    case kTcpWaitEventTypeSslClose:
        return kEventTypeNone;
    default:
        break;
    }
    return kEventTypeNone;
}

bool NetSslWaitEvent::OnWaitPrepare(coroutine::Coroutine *co, void* ctx)
{
    this->m_waitco = co;
    this->m_ctx = ctx;
    switch (m_type)
    {
    case kTcpWaitEventTypeError:
        return false;
    case kTcpWaitEventTypeAccept:
        return OnAcceptWaitPrepare(co, ctx);
    case kTcpWaitEventTypeSslAccept:
        return OnSslAcceptWaitPrepare(co, ctx);
    case kTcpWaitEventTypeRecv:
        return OnRecvWaitPrepare(co, ctx);
    case kTcpWaitEventTypeSslRecv:
        return OnSslRecvWaitPrepare(co, ctx);
    case kTcpWaitEventTypeSend:
        return OnSendWaitPrepare(co, ctx);
    case kTcpWaitEventTypeSslSend:
        return OnSslSendWaitPrepare(co, ctx);
    case kTcpWaitEventTypeConnect:
        return OnConnectWaitPrepare(co, ctx);
    case kTcpWaitEventTypeSslConnect:
        return OnSslConnectWaitPrepare(co, ctx);
    case kTcpWaitEventTypeClose:
        return OnCloseWaitPrepare(co, ctx);
    case kTcpWaitEventTypeSslClose:
        return OnSslCloseWaitPrepare(co, ctx);
    default:
        break;
    }
    return false;
}

void NetSslWaitEvent::HandleEvent(EventEngine *engine)
{
    switch (m_type)
    {
    case kTcpWaitEventTypeError:
        HandleErrorEvent(engine);
        return;
    case kTcpWaitEventTypeAccept:
        HandleAcceptEvent(engine);
        return;
    case kTcpWaitEventTypeSslAccept:
        HandleSslAcceptEvent(engine);
        return;
    case kTcpWaitEventTypeRecv:
        HandleRecvEvent(engine);
        return;
    case kTcpWaitEventTypeSslRecv:
        HandleSslRecvEvent(engine);
        return;
    case kTcpWaitEventTypeSend:
        HandleSendEvent(engine);
        return;
    case kTcpWaitEventTypeSslSend:
        HandleSslSendEvent(engine);
        return;
    case kTcpWaitEventTypeConnect:
        HandleConnectEvent(engine);
        return;
    case kTcpWaitEventTypeSslConnect:
        HandleSslConnectEvent(engine);
        return;
    case kTcpWaitEventTypeClose:
        HandleCloseEvent(engine);
        return;
    case kTcpWaitEventTypeSslClose:
        HandleSslCloseEvent(engine);
        return;
    }
}

async::AsyncSslNetIo *NetSslWaitEvent::GetAsyncTcpSocket() const
{
    return dynamic_cast<async::AsyncSslNetIo*>(m_socket);
}

NetSslWaitEvent::~NetSslWaitEvent()
= default;

bool NetSslWaitEvent::OnSslAcceptWaitPrepare(const coroutine::Coroutine *co, void* ctx)
{
    galay::coroutine::Awaiter* awaiter = co->GetAwaiter();
    SSL* ssl = dynamic_cast<async::AsyncSslNetIo*>(m_socket)->GetSSL();
    SSL_set_accept_state(ssl);
    int r = SSL_do_handshake(ssl);
    LogTrace("[SSL_do_handshake, handle: {}]", m_socket->GetHandle().fd);
    if( r == 1 ){
        awaiter->SetResult(true);
        return false;
    }
    m_ssl_error = SSL_get_error(ssl, r);
    if( m_ssl_error == SSL_ERROR_WANT_READ || m_ssl_error == SSL_ERROR_WANT_WRITE ){
        return true;
    }
    awaiter->SetResult(false);
    m_socket->GetErrorCode() = error::MakeErrorCode(error::Error_SSLAcceptError, errno);
    return false;
}

bool NetSslWaitEvent::OnSslConnectWaitPrepare(const coroutine::Coroutine *co, void* ctx)
{
    
    galay::coroutine::Awaiter* awaiter = co->GetAwaiter();
    SSL* ssl = dynamic_cast<async::AsyncSslNetIo*>(m_socket)->GetSSL();
    SSL_set_connect_state(ssl);
    int r = SSL_do_handshake(ssl);
    LogTrace("[SSL_do_handshake, handle: {}]", m_socket->GetHandle().fd);
    if( r == 1 ){
        awaiter->SetResult(true);
        return false;
    }
    m_ssl_error = SSL_get_error(ssl, r);
    if( m_ssl_error == SSL_ERROR_WANT_READ || m_ssl_error == SSL_ERROR_WANT_WRITE ){
        return true;
    }
    awaiter->SetResult(false);
    m_socket->GetErrorCode() = error::MakeErrorCode(error::Error_SSLConnectError, errno);
    return false;
}

bool NetSslWaitEvent::OnSslRecvWaitPrepare(const coroutine::Coroutine *co, void* ctx)
{
    
    int recvBytes = DealRecv(static_cast<IOVec*>(ctx));
    galay::coroutine::Awaiter* awaiter = co->GetAwaiter();
    if(recvBytes == eCommonNonBlocking) {
        return true;
    }
    awaiter->SetResult(recvBytes);
    return false;
}

bool NetSslWaitEvent::OnSslSendWaitPrepare(const coroutine::Coroutine *co, void* ctx)
{
    
    auto* vec = static_cast<IOVec*>(m_ctx);
    int sendBytes = DealSend(vec);
    galay::coroutine::Awaiter* awaiter = co->GetAwaiter();
    if( sendBytes == eCommonNonBlocking){
        return true;
    }
    awaiter->SetResult(sendBytes);
    return false;
}

bool NetSslWaitEvent::OnSslCloseWaitPrepare(const coroutine::Coroutine *co, void* ctx)
{
    coroutine::Awaiter* awaiter = co->GetAwaiter(); 
    if (m_engine) {
        if( m_engine.load()->DelEvent(this, nullptr) != 0 ) {
            awaiter->SetResult(false);
            m_socket->GetErrorCode() = error::MakeErrorCode(error::Error_DelEventError, errno);
            return false;
        }
    }
    LogTrace("[Close {}]", m_socket->GetHandle().fd);
    int ret = close(m_socket->GetHandle().fd);
    if( ret < 0 ) {
        m_socket->GetErrorCode() = error::MakeErrorCode(error::Error_SSLCloseError, errno);
        awaiter->SetResult(false);
        return false;
    } else {
        awaiter->SetResult(true);
    }
    SSL*& ssl = dynamic_cast<async::AsyncSslNetIo*>(m_socket)->GetSSL();
    SSL_shutdown(ssl);
    SSL_free(ssl);
    ssl = nullptr;
    return false;
}

void NetSslWaitEvent::HandleSslAcceptEvent(EventEngine *engine)
{
    galay::coroutine::Awaiter* awaiter = m_waitco->GetAwaiter();
    SSL* ssl = dynamic_cast<async::AsyncSslNetIo*>(m_socket)->GetSSL();
    const int r = SSL_do_handshake(ssl);
    LogTrace("[SSL_do_handshake, handle: {}]", m_socket->GetHandle().fd);
    if( r == 1 ){
        awaiter->SetResult(true);
        GetCoroutineScheduler(m_socket->GetHandle().fd % GetCoroutineSchedulerNum())->EnqueueCoroutine(m_waitco);
        return;
    }
    m_ssl_error = SSL_get_error(ssl, r);
    if( m_ssl_error == SSL_ERROR_WANT_READ || m_ssl_error == SSL_ERROR_WANT_WRITE ){
        engine->ModEvent(this, nullptr);
    } else {
        awaiter->SetResult(false);
        m_socket->GetErrorCode() = error::MakeErrorCode(error::Error_SSLAcceptError, errno);
        GetCoroutineScheduler(m_socket->GetHandle().fd % GetCoroutineSchedulerNum())->EnqueueCoroutine(m_waitco);
    }
    
}

void NetSslWaitEvent::HandleSslConnectEvent(EventEngine *engine)
{
    galay::coroutine::Awaiter* awaiter = m_waitco->GetAwaiter();
    SSL* ssl = dynamic_cast<async::AsyncSslNetIo*>(m_socket)->GetSSL();
    const int r = SSL_do_handshake(ssl);
    LogTrace("[SSL_do_handshake, handle: {}]", m_socket->GetHandle().fd);
    if( r == 1 ){
        awaiter->SetResult(true);
        GetCoroutineScheduler(m_socket->GetHandle().fd % GetCoroutineSchedulerNum())->EnqueueCoroutine(m_waitco);
    }
    m_ssl_error = SSL_get_error(ssl, r);
    if( m_ssl_error == SSL_ERROR_WANT_READ || m_ssl_error == SSL_ERROR_WANT_WRITE ){
        engine->ModEvent(this, nullptr);
    } else {
        awaiter->SetResult(false);
        m_socket->GetErrorCode() = error::MakeErrorCode(error::Error_SSLConnectError, errno);
        GetCoroutineScheduler(m_socket->GetHandle().fd % GetCoroutineSchedulerNum())->EnqueueCoroutine(m_waitco);
    }
    
}

void NetSslWaitEvent::HandleSslRecvEvent(EventEngine *engine)
{
    int recvBytes = DealRecv(static_cast<IOVec*>(m_ctx));
    galay::coroutine::Awaiter* awaiter = m_waitco->GetAwaiter();
    awaiter->SetResult(recvBytes);
    //2.唤醒协程
    GetCoroutineScheduler(m_socket->GetHandle().fd % GetCoroutineSchedulerNum())->EnqueueCoroutine(m_waitco);
}

void NetSslWaitEvent::HandleSslSendEvent(EventEngine *engine)
{
    int sendBytes = DealSend(static_cast<IOVec*>(m_ctx));
    galay::coroutine::Awaiter* awaiter = m_waitco->GetAwaiter();
    awaiter->SetResult(sendBytes);
    GetCoroutineScheduler(m_socket->GetHandle().fd % GetCoroutineSchedulerNum())->EnqueueCoroutine(m_waitco);
}

void NetSslWaitEvent::HandleSslCloseEvent(EventEngine *engine)
{
    //doing nothing
}

EventType NetSslWaitEvent::CovertSSLErrorToEventType() const
{
    switch (m_ssl_error)
    {
    case SSL_ERROR_NONE:
        return kEventTypeNone;
    case SSL_ERROR_WANT_READ:
        return kEventTypeRead;
    case SSL_ERROR_WANT_WRITE:
        return kEventTypeWrite;
    default: ;
    }
    return kEventTypeNone;
}

int NetSslWaitEvent::DealRecv(IOVec* vec)
{
    SSL* ssl = GetAsyncTcpSocket()->GetSSL();
    size_t recvBytes = vec->m_offset + vec->m_length > vec->m_size ? vec->m_size - vec->m_offset : vec->m_length;
    int length = SSL_read(ssl, vec->m_buffer + vec->m_offset, recvBytes);
    if( length == -1 ) {
        if( errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR ) {
            m_socket->GetErrorCode() = error::MakeErrorCode(error::Error_RecvError, errno);
            return eCommonOtherFailed;
        } else {
            return eCommonNonBlocking;
        }
    }
    LogTrace("[Recv, Handle: {}, Byte: {}\nData: {}]", GetAsyncTcpSocket()->GetHandle().fd, length, std::string_view(vec->m_buffer + vec->m_offset, length));
    vec->m_offset += length;
    return length;
}

int NetSslWaitEvent::DealSend(IOVec* vec)
{
    SSL* ssl = GetAsyncTcpSocket()->GetSSL();
    size_t writeBytes = vec->m_offset + vec->m_length > vec->m_size ? vec->m_size - vec->m_offset : vec->m_length;
    const int length = SSL_write(ssl, vec->m_buffer + vec->m_offset, writeBytes);
    if( length == -1 ) {
        if( errno != EAGAIN && errno != EWOULDBLOCK && errno == EINTR ) {
            m_socket->GetErrorCode() = error::MakeErrorCode(error::Error_SendError, errno);
            return eCommonOtherFailed;
        } else {
            return eCommonNonBlocking;
        }
    }
    LogTrace("[Send, Handle: {}, Byte: {}\nData: {}]", GetAsyncTcpSocket()->GetHandle().fd, length, std::string_view(vec->m_buffer + vec->m_offset, length));
    vec->m_offset += length;
    return length;
}
//#endif

FileIoWaitEvent::FileIoWaitEvent(async::AsyncFileIo *fileio)
    :m_fileio(fileio), m_type(kFileIoWaitEventTypeError)
{
    
}

std::string FileIoWaitEvent::Name()
{
    return "FileIoWaitEvent";
}

bool FileIoWaitEvent::OnWaitPrepare(coroutine::Coroutine *co, void *ctx)
{
    this->m_waitco = co;
    this->m_ctx = ctx;
    switch (m_type)
    {
#ifdef __linux__
    case kFileIoWaitEventTypeLinuxAio:
        return OnAioWaitPrepare(co, ctx);
#endif
    case kFileIoWaitEventTypeRead:
        return OnKReadWaitPrepare(co, ctx);
    case kFileIoWaitEventTypeWrite:
        return OnKWriteWaitPrepare(co, ctx);
    default:
        break;
    }
    return false;
}

void FileIoWaitEvent::HandleEvent(EventEngine *engine)
{
    switch (m_type)
    {
#ifdef __linux__
    case kFileIoWaitEventTypeLinuxAio:
        HandleAioEvent(engine);
        break;
#endif
    case kFileIoWaitEventTypeRead:
        HandleKReadEvent(engine);
        break;
    case kFileIoWaitEventTypeWrite:
        HandleKWriteEvent(engine);
        break;
    default:
        break;
    }
}

EventType FileIoWaitEvent::GetEventType()
{
    switch (m_type)
    {
#ifdef __linux__
    case kFileIoWaitEventTypeLinuxAio:
        return kEventTypeRead;
#endif
    case kFileIoWaitEventTypeRead:
        return kEventTypeRead;
    case kFileIoWaitEventTypeWrite:
        return kEventTypeWrite;
    default: ;
    }
    return kEventTypeNone;
}

GHandle FileIoWaitEvent::GetHandle()
{
    return m_fileio->GetHandle();
}

FileIoWaitEvent::~FileIoWaitEvent()
{
    if(m_engine) {
        m_engine.load()->DelEvent(this, nullptr);
    }
}

#ifdef __linux__
bool FileIoWaitEvent::OnAioWaitPrepare(coroutine::Coroutine *co, void *ctx)
{
    return true;
}


void FileIoWaitEvent::HandleAioEvent(EventEngine* engine)
{
    uint64_t finish_nums = 0;
    int ret = read(m_fileio->GetHandle().fd, &finish_nums, sizeof(uint64_t));
    async::AsyncFileNativeAio* fileio = static_cast<async::AsyncFileNativeAio*>(m_fileio);
    io_event events[DEFAULT_IO_EVENTS_SIZE] = {0};
    int r = io_getevents(fileio->GetIoContext(), 1, DEFAULT_IO_EVENTS_SIZE, events, nullptr);
    LogTrace("[io_getevents return {} events]", r);
    while (r --> 0)
    {
        auto& event = events[r];
        if(event.data != nullptr) {
            static_cast<async::AioCallback*>(event.data)->OnAioComplete(&event);
        }
    }
    if( fileio->IoFinished(finish_nums) ){
        engine->DelEvent(this, nullptr);
        GetCoroutineScheduler(m_fileio->GetHandle().fd % GetCoroutineSchedulerNum())->EnqueueCoroutine(m_waitco);
    }
}
#endif

bool FileIoWaitEvent::OnKReadWaitPrepare(coroutine::Coroutine *co, void *ctx)
{
    auto* vec = static_cast<IOVec*>(ctx);
    size_t readBytes = vec->m_offset + vec->m_length > vec->m_size ? vec->m_size - vec->m_offset : vec->m_length;
    int length = read(m_fileio->GetHandle().fd, vec->m_buffer + vec->m_offset, readBytes);
    if(length < 0){
        if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
            return true;
        } else {
            m_fileio->GetErrorCode() = error::MakeErrorCode(error::Error_FileReadError, errno);
            length = eCommonOtherFailed;
        }        
    }
    LogTrace("[Handle: {}, ReadBytes: {}]", m_fileio->GetHandle().fd, length);
    co->GetAwaiter()->SetResult(length);
    vec->m_offset += length;
    return false;
}

void FileIoWaitEvent::HandleKReadEvent(EventEngine *engine)
{
    auto* vec = static_cast<IOVec*>(m_ctx);
    size_t readBytes = vec->m_offset + vec->m_length > vec->m_size ? vec->m_size - vec->m_offset : vec->m_length;
    int length = read(m_fileio->GetHandle().fd, vec->m_buffer + vec->m_offset, readBytes);
    if( length < 0 ) {
        if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
            length = eCommonNonBlocking;
        } else {
            m_fileio->GetErrorCode() = error::MakeErrorCode(error::Error_FileReadError, errno);
            length = eCommonOtherFailed;
        }
    }
    LogTrace("[Handle: {}, ReadBytes: {}]", m_fileio->GetHandle().fd, length);
    engine->DelEvent(this, nullptr);
    m_waitco->GetAwaiter()->SetResult(length);
    vec->m_offset += length;
    GetCoroutineScheduler(m_fileio->GetHandle().fd % GetCoroutineSchedulerNum())->EnqueueCoroutine(m_waitco);
}

bool FileIoWaitEvent::OnKWriteWaitPrepare(coroutine::Coroutine *co, void *ctx)
{
    auto* vec = static_cast<IOVec*>(ctx);
    size_t writeBytes = vec->m_offset + vec->m_length > vec->m_size ? vec->m_size - vec->m_offset : vec->m_length;
    int length = write(m_fileio->GetHandle().fd, vec->m_buffer + vec->m_offset, writeBytes);
    if(length < 0){
         if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
            return true;
        }else {
            m_fileio->GetErrorCode() = error::MakeErrorCode(error::Error_FileReadError, errno);
            length = eCommonOtherFailed;
        }
    }
    LogTrace("[Handle: {}, WriteBytes: {}]", m_fileio->GetHandle().fd, length);
    vec->m_offset += length;
    co->GetAwaiter()->SetResult(length);
    return false;
}

void FileIoWaitEvent::HandleKWriteEvent(EventEngine *engine)
{
    auto* vec = static_cast<IOVec*>(m_ctx);
    size_t writeBytes = vec->m_offset + vec->m_length > vec->m_size ? vec->m_size - vec->m_offset : vec->m_length;
    int length = write(m_fileio->GetHandle().fd, vec->m_buffer + vec->m_offset, writeBytes);
    if(length < 0){
         if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
            length = eCommonNonBlocking;
        }else {
            m_fileio->GetErrorCode() = error::MakeErrorCode(error::Error_FileReadError, errno);
            length = eCommonOtherFailed;
        }
    }
    LogTrace("[Handle: {}, WriteBytes: {}]", m_fileio->GetHandle().fd, length);
    engine->DelEvent(this, nullptr);
    m_waitco->GetAwaiter()->SetResult(length);
    vec->m_offset += length;
    GetCoroutineScheduler(m_fileio->GetHandle().fd % GetCoroutineSchedulerNum())->EnqueueCoroutine(m_waitco);
}

}