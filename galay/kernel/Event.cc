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

EventEngine*& CallbackEvent::BelongEngine()
{
    return m_engine;
}


CallbackEvent::~CallbackEvent()
{
    if( m_engine ) m_engine->DelEvent(this, nullptr);
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
}

bool TimeEventIDStore::GetEventId(int& id)
{
    return m_eventIds.try_dequeue(id);
}

bool TimeEventIDStore::RecycleEventId(int id)
{
    return m_eventIds.enqueue(id);
}

TimeEventIDStore::~TimeEventIDStore()
{
    free(m_temp);
}

TimeEventIDStore TimeEvent::IDStroe(DEFAULT_TIMEEVENT_ID_CAPACITY);

bool TimeEvent::CreateHandle(GHandle &handle)
{
    return IDStroe.GetEventId(handle.fd);
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
    m_engine->AddEvent(this, nullptr);
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

EventEngine*& TimeEvent::BelongEngine()
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
    m_engine->DelEvent(this, nullptr);
#if defined(__linux__)
    close(m_handle.fd);
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    IDStroe.RecycleEventId(m_handle.fd);
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

ListenEvent::ListenEvent(EventEngine* engine, async::AsyncTcpSocket* socket, TcpCallbackStore* callback_store)
    : m_socket(socket), m_callback_store(callback_store), m_engine(engine), m_action(new action::TcpEventAction(engine, new TcpWaitEvent(socket)))
{
    engine->AddEvent(this, nullptr);
}

void ListenEvent::HandleEvent(EventEngine *engine)
{
    CreateTcpSocket(engine);
    engine->ModEvent(this, nullptr);
}

GHandle ListenEvent::GetHandle()
{
    return m_socket->GetHandle();
}

EventEngine*& ListenEvent::BelongEngine()
{
    return m_engine;
}

ListenEvent::~ListenEvent()
{
    if(m_engine) m_engine->DelEvent(this, nullptr);
    delete m_action;
}

coroutine::Coroutine ListenEvent::CreateTcpSocket(EventEngine *engine)
{
    while(1)
    {
        GHandle new_handle = co_await m_socket->Accept(m_action);
        if( new_handle.fd == -1 ){
            uint32_t error = m_socket->GetErrorCode();
            if( error != error::Error_NoError ) {
                spdlog::error("[{}:{}] [[Accept error] [Errmsg:{}]]", __FILE__, __LINE__, error::GetErrorString(error));
            }
            co_return;
        }
        async::AsyncTcpSocket* socket = new async::AsyncTcpSocket();
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
        TcpWaitEvent* event = new TcpWaitEvent(socket);
        action::TcpEventAction* action = new action::TcpEventAction(engine, event);
        this->m_callback_store->Execute(action);
    }
    co_return;
}

SslListenEvent::SslListenEvent(EventEngine* engine, async::AsyncTcpSslSocket *socket, TcpSslCallbackStore *callback_store)
    :m_socket(socket), m_callback_store(callback_store), m_engine(engine), m_action(new action::TcpSslEventAction(engine, new TcpSslWaitEvent(socket)))
{
    engine->AddEvent(this, nullptr);
}

void SslListenEvent::HandleEvent(EventEngine *engine)
{
    CreateTcpSslSocket(engine);
    engine->ModEvent(this, nullptr);
}

GHandle SslListenEvent::GetHandle()
{
    return m_socket->GetHandle();
}

EventEngine*& SslListenEvent::BelongEngine()
{
    return m_engine;
}

SslListenEvent::~SslListenEvent()
{
    m_engine->DelEvent(this, nullptr);
    delete m_action;
}

coroutine::Coroutine SslListenEvent::CreateTcpSslSocket(EventEngine *engine)
{
    while (true)
    {
        GHandle new_handle = co_await m_socket->Accept(m_action);
        if( new_handle.fd == -1 ){
            uint32_t error = m_socket->GetErrorCode();
            if( error != error::Error_NoError ) {
                spdlog::error("[{}:{}] [[Accept error] [Errmsg:{}]]", __FILE__, __LINE__, error::GetErrorString(error));
            }
            co_return;
        }
        SSL* ssl = async::AsyncTcpSslSocket::SSLSocket(new_handle, GetGlobalSSLCtx());
        if( ssl == nullptr) {
            continue;
        }
        async::AsyncTcpSslSocket* socket = new async::AsyncTcpSslSocket(new_handle, ssl);
        spdlog::debug("[{}:{}] [[Accept success] [Handle:{}]]", __FILE__, __LINE__, socket->GetHandle().fd);
        if (!socket->GetOption().HandleNonBlock())
        {
            close(new_handle.fd);
            spdlog::error("[{}:{}] [[HandleNonBlock error] [Errmsg:{}]]", __FILE__, __LINE__, strerror(errno));
            delete socket;
            co_return;
        }
        event::TcpSslWaitEvent* event = new event::TcpSslWaitEvent(socket);
        action::TcpSslEventAction* action = new action::TcpSslEventAction(engine, event);
        bool success = co_await socket->SSLAccept(action);
        if( !success ){
            close(new_handle.fd);
            spdlog::error("[{}:{}] [[SSLAccept error] [Errmsg:{}]]", __FILE__, __LINE__, strerror(errno));
            delete socket;
            co_return;
        }
        spdlog::debug("[{}:{}] [[SSL_Accept success] [Handle:{}]]", __FILE__, __LINE__, socket->GetHandle().fd);
        engine->ResetMaxEventSize(new_handle.fd);
        this->m_callback_store->Execute(action);
    }
    
}

WaitEvent::WaitEvent(EventEngine* engine)
    :m_waitco(nullptr), m_engine(engine)
{
}

EventEngine*& WaitEvent::BelongEngine()
{
    return m_engine;
}


TcpWaitEvent::TcpWaitEvent(async::AsyncTcpSocket *socket)
    :m_socket(socket), m_type(kWaitEventTypeError)
{
}

std::string TcpWaitEvent::Name()
{
    switch (m_type)
    {
    case kWaitEventTypeError:
        return "ErrorEvent";
    case kWaitEventTypeAccept:
        return "AcceptEvent";
    case kWaitEventTypeRecv:
        return "RecvEvent";
    case kWaitEventTypeSend:
        return "SendEvent";
    case kWaitEventTypeConnect:
        return "ConnectEvent";
    case kWaitEventTypeClose:
        return "CloseEvent";
    default:
        break;
    }
    return "UnknownEvent";
}

bool TcpWaitEvent::OnWaitPrepare(coroutine::Coroutine *co, void* ctx)
{
    this->m_waitco = co;
    switch (m_type)
    {
    case kWaitEventTypeError:
        return false;
    case kWaitEventTypeAccept:
        return OnAcceptWaitPrepare(co, ctx);
    case kWaitEventTypeRecv:
        return OnRecvWaitPrepare(co, ctx);
    case kWaitEventTypeSend:
        return OnSendWaitPrepare(co, ctx);
    case kWaitEventTypeConnect:
        return OnConnectWaitPrepare(co, ctx);
    case kWaitEventTypeClose:
        return OnCloseWaitPrepare(co, ctx);
    default:
        break;
    }
    return false;
}

void TcpWaitEvent::HandleEvent(EventEngine *engine)
{
    switch (m_type)
    {
    case kWaitEventTypeError:
        HandleErrorEvent(engine);
        return;
    case kWaitEventTypeAccept:
        HandleAcceptEvent(engine);
        return;
    case kWaitEventTypeRecv:
        HandleRecvEvent(engine);
        return;
    case kWaitEventTypeSend:
        HandleSendEvent(engine);
        return;
    case kWaitEventTypeConnect:
        HandleConnectEvent(engine);
        return;
    case kWaitEventTypeClose:
        HandleCloseEvent(engine);
        return;
    default:
        return;
    }
}

EventType TcpWaitEvent::GetEventType()
{
    switch (m_type)
    {
    case kWaitEventTypeError:
        return kEventTypeError;
    case kWaitEventTypeAccept:
        return kEventTypeRead;
    case kWaitEventTypeRecv:
        return kEventTypeRead;
    case kWaitEventTypeSend:
        return kEventTypeWrite;
    case kWaitEventTypeConnect:
        return kEventTypeWrite;
    case kWaitEventTypeClose:
        return kEventTypeNone;
    default:
        break;
    }
    return kEventTypeNone;
}

GHandle TcpWaitEvent::GetHandle()
{
    return m_socket->GetHandle();
}

TcpWaitEvent::~TcpWaitEvent()
{
    if(m_engine) m_engine->DelEvent(this, nullptr);
    if(m_socket) delete m_socket;
}

bool TcpWaitEvent::OnAcceptWaitPrepare(coroutine::Coroutine *co, void *ctx)
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

bool TcpWaitEvent::OnRecvWaitPrepare(coroutine::Coroutine *co, void* ctx)
{
    int recvBytes = DealRecv();
    galay::coroutine::Awaiter* awaiter = co->GetAwaiter();
    spdlog::debug("TcpWaitEvent::OnRecvWaitPrepare recvBytes: {}", recvBytes);
    if( recvBytes < 0 ) {
        if( errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR ){
            return true;
        }else {
            recvBytes = -1;
            awaiter->SetResult(recvBytes);
            m_socket->GetErrorCode() = error::MakeErrorCode(error::Error_RecvError, errno);
            return false;
        }
    }
    awaiter->SetResult(recvBytes);
    return false;
}

bool TcpWaitEvent::OnSendWaitPrepare(coroutine::Coroutine *co, void* ctx)
{
    int sendBytes = DealSend();
    spdlog::debug("TcpWaitEvent::OnSendWaitPrepare sendBytes: {}", sendBytes);
    galay::coroutine::Awaiter* awaiter = co->GetAwaiter();
    if( sendBytes < 0){
        if( errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR ){
        }else{
            awaiter->SetResult(sendBytes);
            m_socket->GetErrorCode() = error::MakeErrorCode(error::Error_SendError, errno);
            return false; 
        }
    }
    if (!m_socket->GetWBuffer().empty()){
        return true;
    }
    awaiter->SetResult(sendBytes);
    return false;
}

bool TcpWaitEvent::OnConnectWaitPrepare(coroutine::Coroutine *co, void* ctx)
{
    spdlog::debug("TcpWaitEvent::OnConnectWaitPrepare");
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

bool TcpWaitEvent::OnCloseWaitPrepare(coroutine::Coroutine *co, void* ctx)
{
    spdlog::debug("TcpWaitEvent::OnCloseWaitPrepare");
    coroutine::Awaiter* awaiter = co->GetAwaiter(); 
    if (m_engine) {
        if( m_engine->DelEvent(this, nullptr) != 0 ) {
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

void TcpWaitEvent::HandleErrorEvent(EventEngine *engine)
{
    spdlog::debug("TcpWaitEvent::HandleErrorEvent");
}

void TcpWaitEvent::HandleAcceptEvent(EventEngine *engine)
{
    spdlog::debug("TcpWaitEvent::HandleAcceptEvent");
    // doing nothing
}

void TcpWaitEvent::HandleRecvEvent(EventEngine *engine)
{
    spdlog::debug("TcpWaitEvent::HandleRecvEvent");
    int recvBytes = DealRecv();
    galay::coroutine::Awaiter* awaiter = m_waitco->GetAwaiter();
    awaiter->SetResult(recvBytes);
    //2.唤醒协程
    GetCoroutineScheduler(m_socket->GetHandle().fd % GetCoroutineSchedulerNum())->EnqueueCoroutine(m_waitco);
}

void TcpWaitEvent::HandleSendEvent(EventEngine *engine)
{
    spdlog::debug("TcpWaitEvent::HandleSendEvent");
    int sendBytes = DealSend();
    galay::coroutine::Awaiter* awaiter = m_waitco->GetAwaiter();
    awaiter->SetResult(sendBytes);
    GetCoroutineScheduler(m_socket->GetHandle().fd % GetCoroutineSchedulerNum())->EnqueueCoroutine(m_waitco);
}

void TcpWaitEvent::HandleConnectEvent(EventEngine *engine)
{
    spdlog::debug("TcpWaitEvent::HandleConnectEvent");
    galay::coroutine::Awaiter* awaiter = m_waitco->GetAwaiter();
    awaiter->SetResult(true);
    GetCoroutineScheduler(m_socket->GetHandle().fd % GetCoroutineSchedulerNum())->EnqueueCoroutine(m_waitco);
}

void TcpWaitEvent::HandleCloseEvent(EventEngine *engine)
{ 
    spdlog::debug("TcpWaitEvent::HandleCloseEvent");
    //doing nothing
}

int TcpWaitEvent::DealRecv()
{
    GHandle handle = this->m_socket->GetHandle();
    char* result = nullptr;
    char buffer[DEFAULT_READ_BUFFER_SIZE] = {0};
    int recvBytes = 0;
    bool initial = true;
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
                if (recvBytes == 0) recvBytes = -1;
                m_socket->GetErrorCode() = error::MakeErrorCode(error::Error_RecvError, errno);   
            }
            break;
        } else {
            if(initial) {
                result = new char[ret];
                memset(result, 0, ret);
                initial = false;
            }else{
                result = (char*)realloc(result, ret + recvBytes);
            }
            memcpy(result + recvBytes, buffer, ret);
            recvBytes += ret;
        }
    } while(1); 
    if( recvBytes > 0){
        spdlog::debug("RecvEvent::HandleEvent [Handle: {}] [Byte: {}] [Data: {}]", handle.fd, recvBytes, std::string_view(result, recvBytes));   
        std::string_view origin = m_socket->GetRBuffer();
        char* new_buffer = nullptr;
        if(!origin.empty()) {
            new_buffer = new char[origin.size() + recvBytes];
            memcpy(new_buffer, origin.data(), origin.size());
            memcpy(new_buffer + origin.size(), result, recvBytes);
            delete [] origin.data();
            delete [] result;
        }else{
            new_buffer = result;
        }
        m_socket->SetRBuffer(std::string_view(new_buffer, recvBytes + origin.size()));
    }
    return recvBytes;
}

int TcpWaitEvent::DealSend()
{
    GHandle handle = this->m_socket->GetHandle();
    int sendBytes = 0;
    std::string_view wbuffer = m_socket->GetWBuffer();
    do {
        std::string_view view = wbuffer.substr(sendBytes);
        int ret = send(handle.fd, view.data(), view.size(), 0);
        if( ret == -1 ) {
            if(sendBytes == 0) {
                if( errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR ) {
                    
                }else {
                    m_socket->GetErrorCode() = error::MakeErrorCode(error::Error_SendError, errno);
                }
                return -1;
            }
            break;
        }
        sendBytes += ret;
        if( sendBytes == wbuffer.size() ) {
            break;
        }
    } while (1);
    m_socket->SetWBuffer(wbuffer.substr(sendBytes));
    spdlog::debug("SendEvent::HandleEvent [Handle: {}] [Byte: {}] [Data: {}]", handle.fd, sendBytes, wbuffer.substr(0, sendBytes));
    return sendBytes;
}

TcpSslWaitEvent::TcpSslWaitEvent(async::AsyncTcpSslSocket *socket)
    :TcpWaitEvent(socket)
{
}

std::string TcpSslWaitEvent::Name()
{
    switch (m_type)
    {
    case kWaitEventTypeError:
        return "ErrorEvent";
    case kWaitEventTypeAccept:
        return "AcceptEvent";
    case kWaitEventTypeSslAccept:
        return "SslAcceptEvent";
    case kWaitEventTypeRecv:
        return "RecvEvent";
    case kWaitEventTypeSslRecv:
        return "SslRecvEvent";
    case kWaitEventTypeSend:
        return "SendEvent";
    case kWaitEventTypeSslSend:
        return "SslSendEvent";
    case kWaitEventTypeConnect:
        return "ConnectEvent";
    case kWaitEventTypeSslConnect:
        return "SslConnectEvent";
    case kWaitEventTypeClose:
        return "CloseEvent";
    case kWaitEventTypeSslClose:
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
    case kWaitEventTypeError:
        return kEventTypeError;
    case kWaitEventTypeAccept:
        return kEventTypeRead;
    case kWaitEventTypeSslAccept:
        return CovertSSLErrorToEventType();
    case kWaitEventTypeRecv:
        return kEventTypeRead;
    case kWaitEventTypeSslRecv:
        return kEventTypeRead;
    case kWaitEventTypeSend:
        return kEventTypeWrite;
    case kWaitEventTypeSslSend:
        return kEventTypeWrite;
    case kWaitEventTypeConnect:
        return kEventTypeWrite;
    case kWaitEventTypeSslConnect:
        return CovertSSLErrorToEventType();
    case kWaitEventTypeClose:
        return kEventTypeNone;
    case kWaitEventTypeSslClose:
        return kEventTypeNone;
    default:
        break;
    }
    return kEventTypeNone;
}

bool TcpSslWaitEvent::OnWaitPrepare(coroutine::Coroutine *co, void* ctx)
{
    this->m_waitco = co;
    switch (m_type)
    {
    case kWaitEventTypeError:
        return false;
    case kWaitEventTypeAccept:
        return OnAcceptWaitPrepare(co, ctx);
    case kWaitEventTypeSslAccept:
        return OnSslAcceptWaitPrepare(co, ctx);
    case kWaitEventTypeRecv:
        return OnRecvWaitPrepare(co, ctx);
    case kWaitEventTypeSslRecv:
        return OnSslRecvWaitPrepare(co, ctx);
    case kWaitEventTypeSend:
        return OnSendWaitPrepare(co, ctx);
    case kWaitEventTypeSslSend:
        return OnSslSendWaitPrepare(co, ctx);
    case kWaitEventTypeConnect:
        return OnConnectWaitPrepare(co, ctx);
    case kWaitEventTypeSslConnect:
        return OnSslConnectWaitPrepare(co, ctx);
    case kWaitEventTypeClose:
        return OnCloseWaitPrepare(co, ctx);
    case kWaitEventTypeSslClose:
        return OnSslCloseWaitPrepare(co, ctx);
    default:
        break;
    }
    return false;
}

void TcpSslWaitEvent::HandleEvent(EventEngine *engine)
{
    if(m_waitco == nullptr) {
        spdlog::warn("TcpWaitEvent::HandleEvent m_waitco is nullptr");
        return;
    }
    switch (m_type)
    {
    case kWaitEventTypeError:
        HandleErrorEvent(engine);
        return;
    case kWaitEventTypeAccept:
        HandleAcceptEvent(engine);
        return;
    case kWaitEventTypeSslAccept:
        HandleSslAcceptEvent(engine);
        return;
    case kWaitEventTypeRecv:
        HandleRecvEvent(engine);
        return;
    case kWaitEventTypeSslRecv:
        HandleSslRecvEvent(engine);
        return;
    case kWaitEventTypeSend:
        HandleSendEvent(engine);
        return;
    case kWaitEventTypeSslSend:
        HandleSslSendEvent(engine);
        return;
    case kWaitEventTypeConnect:
        HandleConnectEvent(engine);
        return;
    case kWaitEventTypeSslConnect:
        HandleSslConnectEvent(engine);
        return;
    case kWaitEventTypeClose:
        HandleCloseEvent(engine);
        return;
    case kWaitEventTypeSslClose:
        HandleSslCloseEvent(engine);
        return;
    }
}

async::AsyncTcpSslSocket *TcpSslWaitEvent::GetAsyncTcpSocket()
{
    return static_cast<async::AsyncTcpSslSocket*>(m_socket);
}

TcpSslWaitEvent::~TcpSslWaitEvent()
{
}

bool TcpSslWaitEvent::OnSslAcceptWaitPrepare(coroutine::Coroutine *co, void* ctx)
{
    spdlog::debug("TcpSslWaitEvent::OnSslAcceptWaitPrepare");
    galay::coroutine::Awaiter* awaiter = co->GetAwaiter();
    SSL* ssl = static_cast<async::AsyncTcpSslSocket*>(m_socket)->GetSSL();
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
    SSL* ssl = static_cast<async::AsyncTcpSslSocket*>(m_socket)->GetSSL();
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
    int recvBytes = DealRecv();
    galay::coroutine::Awaiter* awaiter = co->GetAwaiter();
    spdlog::debug("TcpSslWaitEvent::OnSslRecvWaitPrepare recvBytes: {}", recvBytes);
    if( recvBytes < 0 ) {
        if( errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR ){
            return true;
        }else {
            recvBytes = -1;
            awaiter->SetResult(recvBytes);
            m_socket->GetErrorCode() = error::MakeErrorCode(error::Error_RecvError, errno);
            return false;
        }
    }
    awaiter->SetResult(recvBytes);
    return false;
}

bool TcpSslWaitEvent::OnSslSendWaitPrepare(coroutine::Coroutine *co, void* ctx)
{
    int sendBytes = DealSend();
    galay::coroutine::Awaiter* awaiter = co->GetAwaiter();
    spdlog::debug("TcpSslWaitEvent::OnSslSendWaitPrepare sendBytes: {}", sendBytes);
    if( sendBytes < 0){
        if( errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR ){
        }else{
            awaiter->SetResult(sendBytes);
            m_socket->GetErrorCode() = error::MakeErrorCode(error::Error_SendError, errno);
            return false; 
        }
    }
    if (!m_socket->GetWBuffer().empty()){
        return true;
    }
    awaiter->SetResult(sendBytes);
    return false;
}

bool TcpSslWaitEvent::OnSslCloseWaitPrepare(coroutine::Coroutine *co, void* ctx)
{
    spdlog::debug("TcpWaitEvent::OnCloseWaitPrepare");
    coroutine::Awaiter* awaiter = co->GetAwaiter(); 
    if (m_engine) {
        if( m_engine->DelEvent(this, nullptr) != 0 ) {
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
    SSL*& ssl = static_cast<async::AsyncTcpSslSocket*>(m_socket)->GetSSL();
    SSL_shutdown(ssl);
    SSL_free(ssl);
    ssl = nullptr;
    return false;
}

void TcpSslWaitEvent::HandleSslAcceptEvent(EventEngine *engine)
{
    spdlog::debug("TcpSslWaitEvent::HandleSslAcceptEvent");
    galay::coroutine::Awaiter* awaiter = m_waitco->GetAwaiter();
    SSL* ssl = static_cast<async::AsyncTcpSslSocket*>(m_socket)->GetSSL();
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
    SSL* ssl = static_cast<async::AsyncTcpSslSocket*>(m_socket)->GetSSL();
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
    int recvBytes = DealRecv();
    galay::coroutine::Awaiter* awaiter = m_waitco->GetAwaiter();
    awaiter->SetResult(recvBytes);
    //2.唤醒协程
    GetCoroutineScheduler(m_socket->GetHandle().fd % GetCoroutineSchedulerNum())->EnqueueCoroutine(m_waitco);
}

void TcpSslWaitEvent::HandleSslSendEvent(EventEngine *engine)
{
    spdlog::debug("TcpSslWaitEvent::HandleSslSendEvent");
    int sendBytes = DealSend();
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

int TcpSslWaitEvent::DealRecv()
{
    GHandle handle = m_socket->GetHandle();
    SSL* ssl = static_cast<async::AsyncTcpSslSocket*>(m_socket)->GetSSL();
    char* result = nullptr;
    char buffer[DEFAULT_READ_BUFFER_SIZE] = {0};
    int recvBytes = 0;
    bool initial = true;
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
                if (recvBytes == 0) recvBytes = -1;
                m_socket->GetErrorCode() = error::MakeErrorCode(error::Error_RecvError, errno);   
            }
            break;
        } else {
            if(initial) {
                result = new char[ret];
                memset(result, 0, ret);
                initial = false;
            }else{
                result = (char*)realloc(result, ret + recvBytes);
            }
            memcpy(result + recvBytes, buffer, ret);
            recvBytes += ret;
        }
    } while(1); 
    if( recvBytes > 0){
        spdlog::debug("RecvEvent::HandleEvent [Engine: {}] [Handle: {}] [Byte: {}] [Data: {}]", m_engine->GetHandle().fd, handle.fd, recvBytes, std::string_view(result, recvBytes));   
        std::string_view origin = m_socket->GetRBuffer();
        char* new_buffer = nullptr;
        if(!origin.empty()) {
            new_buffer = new char[origin.size() + recvBytes];
            memcpy(new_buffer, origin.data(), origin.size());
            memcpy(new_buffer + origin.size(), result, recvBytes);
            delete [] origin.data();
            delete [] result;
        }else{
            new_buffer = result;
        }
        m_socket->SetRBuffer(std::string_view(new_buffer, recvBytes + origin.size()));
    }
    return recvBytes;
}

int TcpSslWaitEvent::DealSend()
{
    GHandle handle = this->m_socket->GetHandle();
    SSL* ssl = static_cast<async::AsyncTcpSslSocket*>(m_socket)->GetSSL();
    int sendBytes = 0;
    std::string_view wbuffer = m_socket->GetWBuffer();
    do {
        std::string_view view = wbuffer.substr(sendBytes);
        int ret = SSL_write(ssl, view.data(), view.size());
        if( ret == -1 ) {
            if(sendBytes == 0) {
                if( errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR ) {
                    
                }else {
                    m_socket->GetErrorCode() = error::MakeErrorCode(error::Error_SendError, errno);
                }
                return -1;
            }
            break;
        }
        sendBytes += ret;
        if( sendBytes == wbuffer.size() ) {
            break;
        }
    } while (1);
    m_socket->SetWBuffer(wbuffer.substr(sendBytes));
    spdlog::debug("SendEvent::HandleEvent [Engine: {}] [Handle: {}] [Byte: {}] [Data: {}]", m_engine->GetHandle().fd, handle.fd, sendBytes, wbuffer.substr(0, sendBytes));
    return sendBytes;
}
//#endif


}