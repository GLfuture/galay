#include "Event.h"
#include "Async.h"
#include "EventEngine.h"
#include "Scheduler.h"
#include "Operation.h"
#include "galay/util/Time.h"
#include "ExternApi.h"
#include <string.h>
#if defined(__linux__)
#include <sys/timerfd.h>
#elif  defined(WIN32) || defined(_WIN32) || defined(_WIN32_) || defined(WIN64) || defined(_WIN64) || defined(_WIN64_)

#endif
#include <spdlog/spdlog.h>

namespace galay::event
{

std::string ToString(EventType type)
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

    
CallbackEvent::CallbackEvent(GHandle handle, EventType type, std::function<void(Event*, EventEngine*)> callback)
    : m_handle(handle), m_type(type), m_callback(callback), m_engine(nullptr)
{
    
}

void CallbackEvent::HandleEvent(EventEngine *engine)
{
    this->m_callback(this, engine);
}

bool CallbackEvent::SetEventEngine(EventEngine *engine)
{
    event::EventEngine* t = m_engine.load();
    if(!m_engine.compare_exchange_strong(t, engine)) {
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
Timer::Timer(int64_t during_time, std::function<void(Timer::ptr)> &&func)
{
    this->m_rightHandle = std::forward<std::function<void(Timer::ptr)>>(func);
    SetDuringTime(during_time);
}

int64_t 
Timer::GetDuringTime()
{
    return this->m_duringTime;
}

int64_t 
Timer::GetExpiredTime()
{
    return this->m_expiredTime;
}

int64_t 
Timer::GetRemainTime()
{
    int64_t time = this->m_expiredTime - time::GetCurrentTime();
    return time < 0 ? 0 : time;
}

void 
Timer::SetDuringTime(int64_t duringTime)
{
    this->m_duringTime = duringTime;
    this->m_expiredTime = time::GetCurrentTime() + duringTime;
    this->m_success = false;
}

void 
Timer::Execute()
{
    if ( m_cancle.load() ) return;
    this->m_rightHandle(shared_from_this());
    this->m_success = true;
}

void 
Timer::Cancle()
{
    this->m_cancle = true;
}

// 是否已经完成
bool 
Timer::Success()
{
    return this->m_success.load();
}

bool 
Timer::TimerCompare::operator()(const Timer::ptr &a, const Timer::ptr &b)
{
    if (a->GetExpiredTime() > b->GetExpiredTime())
    {
        return true;
    }
    return false;
}

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
TimeEventIDStore::TimeEventIDStore(int capacity)
{
    m_temp = (int*)malloc(sizeof(int) * capacity);
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

bool TimeEventIDStore::RecycleEventId(int id)
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


TimeEvent::TimeEvent(GHandle handle, EventEngine* engine)
    : m_handle(handle), m_engine(engine)
{
#if defined(__linux__)
    m_engine.load()->AddEvent(this, nullptr);
#endif
}

void TimeEvent::HandleEvent(EventEngine *engine)
{
#if defined(__linux__)
    std::vector<Timer::ptr> timers;
    std::unique_lock lock(this->m_mutex);
    while (! m_timers.empty() && m_timers.top()->GetExpiredTime()  <= time::GetCurrentTime() ) {
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

Timer::ptr TimeEvent::AddTimer(int64_t during_time, std::function<void(Timer::ptr)> &&func)
{
    auto timer = std::make_shared<Timer>(during_time, std::forward<std::function<void(Timer::ptr)>>(func));
    std::unique_lock<std::shared_mutex> lock(this->m_mutex);
    this->m_timers.push(timer);
    lock.unlock();
#if defined(__linux__)
    UpdateTimers();
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    m_engine->ModEvent(this, timer.get());
#endif
    
    return timer;
}

void TimeEvent::ReAddTimer(int64_t during_time, Timer::ptr timer)
{
    timer->SetDuringTime(during_time);
    timer->m_cancle.store(false);
    std::unique_lock lock(this->m_mutex);
    this->m_timers.push(timer);
    lock.unlock();
#if defined(__linux__)
    UpdateTimers();
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    m_engine->ModEvent(this, timer.get());
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
    async::NetAddr addr;
    while(1)
    {
        GHandle new_handle = co_await async::AsyncAccept(m_socket, &addr);
        if( new_handle.fd == -1 ){
            uint32_t error = m_socket->GetErrorCode();
            if( error != error::Error_NoError ) {
                spdlog::error("[{}:{}] [[Accept error] [Errmsg:{}]]", __FILE__, __LINE__, error::GetErrorString(error));
            }
            co_return;
        }
        async::AsyncNetIo* socket = new async::AsyncTcpSocket(engine);
        socket->GetHandle() = new_handle;
        spdlog::debug("[{}:{}] [[Accept success] [Handle:{}]]", __FILE__, __LINE__, socket->GetHandle().fd);
        if (!socket->GetOption().HandleNonBlock())
        {
            close(new_handle.fd);
            spdlog::error("[{}:{}] [[HandleNonBlock error] [Errmsg:{}]]", __FILE__, __LINE__, strerror(errno));
            delete socket;
            co_return;
        }
        engine->ResetMaxEventSize(new_handle.fd);
        this->m_callback_store->Execute(socket);
    }
    co_return;
}

SslListenEvent::SslListenEvent(EventEngine* engine, async::AsyncSslNetIo *socket, TcpSslCallbackStore *callback_store)
    :m_socket(socket), m_callback_store(callback_store), m_engine(engine), m_action(new action::SslNetEventAction(engine, new TcpSslWaitEvent(socket)))
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
    async::NetAddr addr;
    while (true)
    {
        GHandle new_handle = co_await async::AsyncAccept(m_socket, &addr);
        if( new_handle.fd == -1 ){
            uint32_t error = m_socket->GetErrorCode();
            if( error != error::Error_NoError ) {
                spdlog::error("[{}:{}] [[Accept error] [Errmsg:{}]]", __FILE__, __LINE__, error::GetErrorString(error));
            }
            co_return;
        }
        async::AsyncSslNetIo* socket = new async::AsyncSslTcpSocket(engine);
        bool res = async::AsyncSSLSocket(socket, GetGlobalSSLCtx());
        if( !res ) {
            delete socket;
            continue;
        }
        spdlog::debug("[{}:{}] [[Accept success] [Handle:{}]]", __FILE__, __LINE__, socket->GetHandle().fd);
        if (!socket->GetOption().HandleNonBlock())
        {
            close(new_handle.fd);
            spdlog::error("[{}:{}] [[HandleNonBlock error] [Errmsg:{}]]", __FILE__, __LINE__, strerror(errno));
            delete socket;
            co_return;
        }
        bool success = co_await async::AsyncSSLAccept(socket);
        if( !success ){
            close(new_handle.fd);
            spdlog::error("[{}:{}] [[SSLAccept error] [Errmsg:{}]]", __FILE__, __LINE__, strerror(errno));
            delete socket;
            co_return;
        }
        spdlog::debug("[{}:{}] [[SSL_Accept success] [Handle:{}]]", __FILE__, __LINE__, socket->GetHandle().fd);
        engine->ResetMaxEventSize(new_handle.fd);
        this->m_callback_store->Execute(socket);
    }
    
}

WaitEvent::WaitEvent()
    :m_waitco(nullptr), m_engine(nullptr)
{
}

bool WaitEvent::SetEventEngine(EventEngine *engine)
{
    event::EventEngine* t = m_engine.load();
    if(!m_engine.compare_exchange_strong(t, engine)) {
        return false;
    }
    return true;
}

EventEngine* WaitEvent::BelongEngine()
{
    return m_engine.load();
}


NetWaitEvent::NetWaitEvent(async::AsyncNetIo *socket)
    :m_socket(socket), m_type(kTcpWaitEventTypeError)
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
        return;
    default:
        return;
    }
}

EventType NetWaitEvent::GetEventType()
{
    switch (m_type)
    {
    case kTcpWaitEventTypeError:
        return kEventTypeError;
    case kTcpWaitEventTypeAccept:
        return kEventTypeRead;
    case kTcpWaitEventTypeRecv:
        return kEventTypeRead;
    case kTcpWaitEventTypeSend:
        return kEventTypeWrite;
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

bool NetWaitEvent::OnAcceptWaitPrepare(coroutine::Coroutine *co, void *ctx)
{
    sockaddr addr;
    socklen_t addrlen = sizeof(addr);
    GHandle handle {
        .fd = accept(m_socket->GetHandle().fd, &addr, &addrlen),
    };
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

bool NetWaitEvent::OnRecvWaitPrepare(coroutine::Coroutine *co, void* ctx)
{
    async::NetIOVec* iov = (async::NetIOVec*)ctx;
    int recvBytes = DealRecv(iov);
    galay::coroutine::Awaiter* awaiter = co->GetAwaiter();
    spdlog::debug("NetWaitEvent::OnRecvWaitPrepare recvBytes: {}", recvBytes);
    if( recvBytes == -1) {
        return true;
    }
    awaiter->SetResult(recvBytes);
    return false;
}

bool NetWaitEvent::OnSendWaitPrepare(coroutine::Coroutine *co, void* ctx)
{
    async::NetIOVec* iov = (async::NetIOVec*)ctx;
    int sendBytes = DealSend(iov);
    spdlog::debug("NetWaitEvent::OnSendWaitPrepare sendBytes: {}", sendBytes);
    galay::coroutine::Awaiter* awaiter = co->GetAwaiter();
    if( sendBytes < 0){
        return false;
    }
    if (sendBytes != iov->m_len){
        return true;
    }
    awaiter->SetResult(sendBytes);
    return false;
}

bool NetWaitEvent::OnConnectWaitPrepare(coroutine::Coroutine *co, void* ctx)
{
    
    spdlog::debug("NetWaitEvent::OnConnectWaitPrepare");
    async::NetAddr* addr = static_cast<async::NetAddr*>(ctx);
    sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = inet_addr(addr->m_ip.c_str());
    saddr.sin_port = htons(addr->m_port);
    int ret = connect(m_socket->GetHandle().fd, (sockaddr*) &saddr, sizeof(sockaddr_in));
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

bool NetWaitEvent::OnCloseWaitPrepare(coroutine::Coroutine *co, void* ctx)
{
    
    spdlog::debug("NetWaitEvent::OnCloseWaitPrepare");
    coroutine::Awaiter* awaiter = co->GetAwaiter(); 
    if (m_engine) {
        if( m_engine.load()->DelEvent(this, nullptr) != 0 ) {
            awaiter->SetResult(false);
            m_socket->GetErrorCode() = error::MakeErrorCode(error::Error_CloseError, errno);
            return false;
        }
    }
    int ret = close(m_socket->GetHandle().fd);
    if( ret < 0 ) {
        m_socket->GetErrorCode() = error::MakeErrorCode(error::Error_CloseError, errno);
        awaiter->SetResult(false);
    } else {
        awaiter->SetResult(true);
    }
    return false;
}

void NetWaitEvent::HandleErrorEvent(EventEngine *engine)
{
    spdlog::debug("NetWaitEvent::HandleErrorEvent");
}

void NetWaitEvent::HandleAcceptEvent(EventEngine *engine)
{
    spdlog::debug("NetWaitEvent::HandleAcceptEvent");
    // doing nothing
}

void NetWaitEvent::HandleRecvEvent(EventEngine *engine)
{
    spdlog::debug("NetWaitEvent::HandleRecvEvent");
    int recvBytes = DealRecv((async::NetIOVec*)m_ctx);
    galay::coroutine::Awaiter* awaiter = m_waitco->GetAwaiter();
    awaiter->SetResult(recvBytes);
    //2.唤醒协程
    GetCoroutineScheduler(m_socket->GetHandle().fd % GetCoroutineSchedulerNum())->EnqueueCoroutine(m_waitco);
}

void NetWaitEvent::HandleSendEvent(EventEngine *engine)
{
    spdlog::debug("NetWaitEvent::HandleSendEvent");
    int sendBytes = DealSend((async::NetIOVec*)m_ctx);
    galay::coroutine::Awaiter* awaiter = m_waitco->GetAwaiter();
    awaiter->SetResult(sendBytes);
    GetCoroutineScheduler(m_socket->GetHandle().fd % GetCoroutineSchedulerNum())->EnqueueCoroutine(m_waitco);
}

void NetWaitEvent::HandleConnectEvent(EventEngine *engine)
{
    spdlog::debug("NetWaitEvent::HandleConnectEvent");
    galay::coroutine::Awaiter* awaiter = m_waitco->GetAwaiter();
    awaiter->SetResult(true);
    GetCoroutineScheduler(m_socket->GetHandle().fd % GetCoroutineSchedulerNum())->EnqueueCoroutine(m_waitco);
}

void NetWaitEvent::HandleCloseEvent(EventEngine *engine)
{ 
    spdlog::debug("NetWaitEvent::HandleCloseEvent");
    //doing nothing
}

int NetWaitEvent::DealRecv(async::NetIOVec* iovc)
{
    GHandle handle = this->m_socket->GetHandle();
    char buffer[DEFAULT_READ_BUFFER_SIZE] = {0};
    int recvBytes = 0;
    do{
        bzero(buffer, sizeof(buffer));
        int ret = recv(handle.fd, buffer, DEFAULT_READ_BUFFER_SIZE, 0);
        if( ret == 0 ) {
            recvBytes = 0;
            break;
        }
        if( ret == -1 ) {
            if( errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR ){
                if (recvBytes == 0) recvBytes = -1;
            }else {
                if (recvBytes == 0) recvBytes = -2;
                m_socket->GetErrorCode() = error::MakeErrorCode(error::Error_RecvError, errno);   
            }
            break;
        } else {
            recvBytes += ret;
            if(recvBytes > iovc->m_len) {
                iovc->m_buf = (char*)realloc(iovc->m_buf, recvBytes);
            } else {
                memcpy(iovc->m_buf + (recvBytes - ret), buffer, ret);
            }
            iovc->m_len = recvBytes;
        }
    } while(1); 
    if( recvBytes > 0){
        spdlog::debug("RecvEvent::HandleEvent [Handle: {}] [Byte: {}] [Data: {}]", handle.fd, recvBytes, std::string_view(iovc->m_buf + iovc->m_len - recvBytes, recvBytes));   
    }
    return recvBytes;
}

int NetWaitEvent::DealSend(async::NetIOVec* iovc)
{
    GHandle handle = this->m_socket->GetHandle();
    int sendBytes = 0;
    do {
        int ret = send(handle.fd, iovc->m_buf, iovc->m_len, 0);
        if( ret == -1 ) {
            if( errno != EAGAIN && errno != EWOULDBLOCK && errno == EINTR ) {
                m_socket->GetErrorCode() = error::MakeErrorCode(error::Error_SendError, errno);
                return -1;
            }
            break;
        }
        sendBytes += ret;
        if( sendBytes == iovc->m_len ) {
            break;
        }
    } while (1);
    spdlog::debug("SendEvent::Deal [Handle: {}] [Byte: {}] [Data: {}]", handle.fd, sendBytes, std::string_view(iovc->m_buf, sendBytes));
    return sendBytes;
}

TcpSslWaitEvent::TcpSslWaitEvent(async::AsyncSslNetIo *socket)
    :NetWaitEvent(socket)
{
}

std::string TcpSslWaitEvent::Name()
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

EventType TcpSslWaitEvent::GetEventType()
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
        return kEventTypeRead;
    case kTcpWaitEventTypeSslRecv:
        return kEventTypeRead;
    case kTcpWaitEventTypeSend:
        return kEventTypeWrite;
    case kTcpWaitEventTypeSslSend:
        return kEventTypeWrite;
    case kTcpWaitEventTypeConnect:
        return kEventTypeWrite;
    case kTcpWaitEventTypeSslConnect:
        return CovertSSLErrorToEventType();
    case kTcpWaitEventTypeClose:
        return kEventTypeNone;
    case kTcpWaitEventTypeSslClose:
        return kEventTypeNone;
    default:
        break;
    }
    return kEventTypeNone;
}

bool TcpSslWaitEvent::OnWaitPrepare(coroutine::Coroutine *co, void* ctx)
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

void TcpSslWaitEvent::HandleEvent(EventEngine *engine)
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

async::AsyncSslNetIo *TcpSslWaitEvent::GetAsyncTcpSocket()
{
    return static_cast<async::AsyncSslNetIo*>(m_socket);
}

TcpSslWaitEvent::~TcpSslWaitEvent()
{
}

bool TcpSslWaitEvent::OnSslAcceptWaitPrepare(coroutine::Coroutine *co, void* ctx)
{
    
    spdlog::debug("TcpSslWaitEvent::OnSslAcceptWaitPrepare");
    galay::coroutine::Awaiter* awaiter = co->GetAwaiter();
    SSL* ssl = static_cast<async::AsyncSslNetIo*>(m_socket)->GetSSL();
    SSL_set_accept_state(ssl);
    int r = SSL_do_handshake(ssl);
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

bool TcpSslWaitEvent::OnSslConnectWaitPrepare(coroutine::Coroutine *co, void* ctx)
{
    
    galay::coroutine::Awaiter* awaiter = co->GetAwaiter();
    SSL* ssl = static_cast<async::AsyncSslNetIo*>(m_socket)->GetSSL();
    SSL_set_connect_state(ssl);
    int r = SSL_do_handshake(ssl);
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

bool TcpSslWaitEvent::OnSslRecvWaitPrepare(coroutine::Coroutine *co, void* ctx)
{
    
    int recvBytes = DealRecv((async::NetIOVec*) ctx);
    galay::coroutine::Awaiter* awaiter = co->GetAwaiter();
    spdlog::debug("TcpSslWaitEvent::OnSslRecvWaitPrepare recvBytes: {}", recvBytes);
    if( recvBytes == -1) {
        return true;
    }
    awaiter->SetResult(recvBytes);
    return false;
}

bool TcpSslWaitEvent::OnSslSendWaitPrepare(coroutine::Coroutine *co, void* ctx)
{
    
    async::NetIOVec* iovc = (async::NetIOVec*) m_ctx;
    int sendBytes = DealSend(iovc);
    galay::coroutine::Awaiter* awaiter = co->GetAwaiter();
    spdlog::debug("TcpSslWaitEvent::OnSslSendWaitPrepare sendBytes: {}", sendBytes);
    if( sendBytes < 0){
        return false;
    }
    if (sendBytes != iovc->m_len){
        return true;
    }
    awaiter->SetResult(sendBytes);
    return false;
}

bool TcpSslWaitEvent::OnSslCloseWaitPrepare(coroutine::Coroutine *co, void* ctx)
{
    
    spdlog::debug("NetWaitEvent::OnCloseWaitPrepare");
    coroutine::Awaiter* awaiter = co->GetAwaiter(); 
    if (m_engine) {
        if( m_engine.load()->DelEvent(this, nullptr) != 0 ) {
            awaiter->SetResult(false);
            m_socket->GetErrorCode() = error::MakeErrorCode(error::Error_DelEventError, errno);
            return false;
        }
    }
    int ret = close(m_socket->GetHandle().fd);
    if( ret < 0 ) {
        m_socket->GetErrorCode() = error::MakeErrorCode(error::Error_SSLCloseError, errno);
        awaiter->SetResult(false);
        return false;
    } else {
        awaiter->SetResult(true);
    }
    SSL*& ssl = static_cast<async::AsyncSslNetIo*>(m_socket)->GetSSL();
    SSL_shutdown(ssl);
    SSL_free(ssl);
    ssl = nullptr;
    return false;
}

void TcpSslWaitEvent::HandleSslAcceptEvent(EventEngine *engine)
{
    spdlog::debug("TcpSslWaitEvent::HandleSslAcceptEvent");
    galay::coroutine::Awaiter* awaiter = m_waitco->GetAwaiter();
    SSL* ssl = static_cast<async::AsyncSslNetIo*>(m_socket)->GetSSL();
    int r = SSL_do_handshake(ssl);
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

void TcpSslWaitEvent::HandleSslConnectEvent(EventEngine *engine)
{
    galay::coroutine::Awaiter* awaiter = m_waitco->GetAwaiter();
    SSL* ssl = static_cast<async::AsyncSslNetIo*>(m_socket)->GetSSL();
    int r = SSL_do_handshake(ssl);
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

void TcpSslWaitEvent::HandleSslRecvEvent(EventEngine *engine)
{
    spdlog::debug("TcpSslWaitEvent::HandleSslRecvEvent");
    int recvBytes = DealRecv((async::NetIOVec*) m_ctx);
    galay::coroutine::Awaiter* awaiter = m_waitco->GetAwaiter();
    awaiter->SetResult(recvBytes);
    //2.唤醒协程
    GetCoroutineScheduler(m_socket->GetHandle().fd % GetCoroutineSchedulerNum())->EnqueueCoroutine(m_waitco);
}

void TcpSslWaitEvent::HandleSslSendEvent(EventEngine *engine)
{
    spdlog::debug("TcpSslWaitEvent::HandleSslSendEvent");
    int sendBytes = DealSend((async::NetIOVec*) m_ctx);
    galay::coroutine::Awaiter* awaiter = m_waitco->GetAwaiter();
    awaiter->SetResult(sendBytes);
    GetCoroutineScheduler(m_socket->GetHandle().fd % GetCoroutineSchedulerNum())->EnqueueCoroutine(m_waitco);
}

void TcpSslWaitEvent::HandleSslCloseEvent(EventEngine *engine)
{
    //doing nothing
}

EventType TcpSslWaitEvent::CovertSSLErrorToEventType()
{
    switch (m_ssl_error)
    {
    case SSL_ERROR_NONE:
        return kEventTypeNone;
    case SSL_ERROR_WANT_READ:
        return kEventTypeRead;
    case SSL_ERROR_WANT_WRITE:
        return kEventTypeWrite;
    }
    return kEventTypeNone;
}

int TcpSslWaitEvent::DealRecv(async::NetIOVec* iovc)
{
    SSL* ssl = GetAsyncTcpSocket()->GetSSL();
    char buffer[DEFAULT_READ_BUFFER_SIZE] = {0};
    int recvBytes = 0;
    do{
        bzero(buffer, sizeof(buffer));
        int ret = SSL_read(ssl, buffer, DEFAULT_READ_BUFFER_SIZE);
        if( ret == 0 ) {
            recvBytes = 0;
            break;
        }
        if( ret == -1 ) {
            if( errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR ){
                if (recvBytes == 0) recvBytes = -1;
            }else {
                if (recvBytes == 0) recvBytes = -2;
                m_socket->GetErrorCode() = error::MakeErrorCode(error::Error_RecvError, errno);   
            }
            break;
        } else {
            recvBytes += ret;
            if(recvBytes > iovc->m_len) {
                iovc->m_buf = (char*)realloc(iovc->m_buf, recvBytes);
            } else {
                memcpy(iovc->m_buf + (recvBytes - ret), buffer, ret);
            }
            iovc->m_len = recvBytes;
        }
    } while(1); 
    if( recvBytes > 0){
        spdlog::debug("RecvEvent::HandleEvent [Handle: {}] [Byte: {}] [Data: {}]", m_socket->GetHandle().fd, recvBytes, std::string_view(iovc->m_buf + iovc->m_len - recvBytes, recvBytes));   
    }
    return recvBytes;
}

int TcpSslWaitEvent::DealSend(async::NetIOVec* iovc)
{
    SSL* ssl = GetAsyncTcpSocket()->GetSSL();
    int sendBytes = 0;
    do {
        int ret = SSL_write(ssl, iovc->m_buf, iovc->m_len);
        if( ret == -1 ) {
            if( errno != EAGAIN && errno != EWOULDBLOCK && errno == EINTR ) {
                m_socket->GetErrorCode() = error::MakeErrorCode(error::Error_SendError, errno);
                return -1;
            }
            break;
        }
        sendBytes += ret;
        if( sendBytes == iovc->m_len ) {
            break;
        }
    } while (1);
    spdlog::debug("SendEvent::DealSend [Handle: {}] [Byte: {}] [Data: {}]", m_socket->GetHandle().fd, sendBytes, std::string_view(iovc->m_buf, sendBytes));
    return sendBytes;
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
            return OnFileIoAioPrepare(co, ctx);
    #endif
        case kFileIoWaitEventTypeRead:
            break;
        case kFileIoWaitEventTypeWrite:
            
            break;
        default:
            break;
    }

    return true;
}

void FileIoWaitEvent::HandleEvent(EventEngine *engine)
{
    switch (m_type)
    {
#ifdef __linux__
    case kFileIoWaitEventTypeLinuxAio:
        OnFileIoAioHandle(engine);
        break;
#endif
    default:
        break;
    }
}

EventType FileIoWaitEvent::GetEventType()
{
    switch (m_type)
    {
    case kFileIoWaitEventTypeLinuxAio:
        return kEventTypeRead;
    default:
        break;
    }
    return kEventTypeRead;
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
bool FileIoWaitEvent::OnFileIoAioPrepare(coroutine::Coroutine *co, void *ctx)
{
    return true;
}


void FileIoWaitEvent::OnFileIoAioHandle(EventEngine* engine)
{
    uint64_t finish_nums = 0;
    read(m_fileio->GetHandle().fd, &finish_nums, sizeof(uint64_t));
    async::AsyncFileNativeAio* fileio = static_cast<async::AsyncFileNativeAio*>(m_fileio);
    io_event events[DEFAULT_IO_EVENTS_SIZE] = {0};
    int r = io_getevents(fileio->GetIoContext(), 1, DEFAULT_IO_EVENTS_SIZE, events, nullptr);
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
}