#include "Event.h"
#include "Async.h"
#include "EventEngine.h"
#include "Scheduler.h"
#include "Operation.h"
#include "../util/Time.h"
#include "Step.h"
#include <string.h>
#if defined(__linux__)
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#elif  defined(WIN32) || defined(_WIN32) || defined(_WIN32_) || defined(WIN64) || defined(_WIN64) || defined(_WIN64_)

#endif
#include <spdlog/spdlog.h>

namespace galay::event
{
    
CallbackEvent::CallbackEvent(GHandle handle, EventType type, std::function<void(Event*, EventEngine*)> callback)
    : m_handle(handle), m_type(type), m_callback(callback), m_event_in_engine(false)
{
    
}

void CallbackEvent::HandleEvent(EventEngine *engine)
{
    this->m_callback(this, engine);
}

void CallbackEvent::Free(EventEngine *engine)
{
    if (m_event_in_engine) engine->DelEvent(this); 
    delete this;
}

CoroutineEvent::CoroutineEvent(GHandle handle, EventEngine* engine, EventType type)
    : m_handle(handle), m_engine(engine), m_type(type), m_event_in_engine(false)
{
}

void CoroutineEvent::HandleEvent(EventEngine *engine)
{
    eventfd_t val;
    eventfd_read(m_handle.fd, &val);
    std::unique_lock lock(this->m_mtx);
    while (!m_coroutines.empty())
    {
        auto co = m_coroutines.front();
        m_coroutines.pop();
        lock.unlock();
        co->Resume();
        lock.lock();
    }
    engine->ModEvent(this);
}

void CoroutineEvent::Free(EventEngine *engine)
{
    if( m_event_in_engine ) engine->DelEvent(this);  
    delete this;
}

void CoroutineEvent::ResumeCoroutine(coroutine::Coroutine *coroutine)
{
    spdlog::debug("CoroutineEvent::ResumeCoroutine [Engine: {}] [Handle:{}]]", m_engine->GetHandle().fd, m_handle.fd);
    std::unique_lock lock(this->m_mtx);
    m_coroutines.push(coroutine);
    lock.unlock();
    eventfd_write(m_handle.fd, 1);
}

ListenEvent::ListenEvent(async::AsyncTcpSocket* socket, CallbackStore* callback_store, scheduler::EventScheduler* net_scheduler, scheduler::CoroutineScheduler* co_schedule)
    : m_socket(socket), m_callback_store(callback_store), m_net_scheduler(net_scheduler), m_co_scheduler(co_schedule), m_event_in_engine(false), m_net_event(new NetWaitEvent(nullptr, socket)), m_action(new action::NetIoEventAction(m_net_event))
{
    async::HandleOption option(this->m_socket->GetHandle());
    option.HandleNonBlock();
}

void ListenEvent::HandleEvent(EventEngine *engine)
{
    CreateTcpSocket(engine);
    engine->ModEvent(this);
}

GHandle ListenEvent::GetHandle()
{
    return m_socket->GetHandle();
}

void ListenEvent::Free(EventEngine *engine)
{
    if( m_event_in_engine ) engine->DelEvent(this);  
    delete this;
}

ListenEvent::~ListenEvent()
{
    delete m_net_event;
    delete m_action;
}

coroutine::Coroutine ListenEvent::CreateTcpSocket(EventEngine *engine)
{
#if defined(USE_EPOLL)
    while(1)
    {
        async::AsyncTcpSocket* socket = new async::AsyncTcpSocket();
        GHandle new_handle = co_await socket->Accept(m_action);
        socket->GetHandle() = new_handle;
        if( new_handle.fd == -1 ){
            std::string error = socket->GetLastError();
            if( !error.empty() ) {
                spdlog::error("[{}:{}] [[Accept error] [Errmsg:{}]]", __FILE__, __LINE__, error);
            }
            delete socket;
            co_return;
        }
        spdlog::debug("[{}:{}] [[Accept success] [Handle:{}]]", __FILE__, __LINE__, socket->GetHandle().fd);
        if (!socket->GetOption().HandleNonBlock())
        {
            closesocket(new_handle.fd);
            spdlog::error("[{}:{}] [[HandleNonBlock error] [Errmsg:{}]]", __FILE__, __LINE__, strerror(errno));
            delete socket;
            co_return;
        }
        // 动态修改event max size
        AfterCreateTcpSocketStep::Execute(socket->GetHandle());
        this->m_callback_store->Execute(socket, m_net_scheduler, m_co_scheduler);
    }
#endif
    co_return;
}

TimeEvent::Timer::Timer(int64_t during_time, std::function<void(Timer::ptr)> &&func)
{
    this->m_rightHandle = std::forward<std::function<void(Timer::ptr)>>(func);
    SetDuringTime(during_time);
}

int64_t 
TimeEvent::Timer::GetDuringTime()
{
    return this->m_duringTime;
}

int64_t 
TimeEvent::Timer::GetExpiredTime()
{
    return this->m_expiredTime;
}

int64_t 
TimeEvent::Timer::GetRemainTime()
{
    int64_t time = this->m_expiredTime - time::GetCurrentTime();
    return time < 0 ? 0 : time;
}

void 
TimeEvent::Timer::SetDuringTime(int64_t duringTime)
{
    this->m_duringTime = duringTime;
    this->m_expiredTime = time::GetCurrentTime() + duringTime;
    this->m_success = false;
}

void 
TimeEvent::Timer::Execute()
{
    if ( m_cancle.load() ) return;
    this->m_rightHandle(shared_from_this());
    this->m_success = true;
}

void 
TimeEvent::Timer::Cancle()
{
    this->m_cancle = true;
}

// 是否已经完成
bool 
TimeEvent::Timer::Success()
{
    return this->m_success.load();
}

bool 
TimeEvent::Timer::TimerCompare::operator()(const Timer::ptr &a, const Timer::ptr &b)
{
    if (a->GetExpiredTime() > b->GetExpiredTime())
    {
        return true;
    }
    return false;
}

TimeEvent::TimeEvent(GHandle handle)
    : m_handle(handle), m_event_in_engine(false)
{
    
}

void TimeEvent::HandleEvent(EventEngine *engine)
{
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
    engine->ModEvent(this);
}

void TimeEvent::Free(EventEngine *engine)
{
    if( m_event_in_engine ) engine->DelEvent(this);
    delete this;
}

TimeEvent::Timer::ptr TimeEvent::AddTimer(int64_t during_time, std::function<void(Timer::ptr)> &&func)
{
    auto timer = std::make_shared<Timer>(during_time, std::forward<std::function<void(Timer::ptr)>>(func));
    std::unique_lock<std::shared_mutex> lock(this->m_mutex);
    this->m_timers.push(timer);
    lock.unlock();
    UpdateTimers();
    return timer;
}

void TimeEvent::ReAddTimer(int64_t during_time, Timer::ptr timer)
{
    timer->SetDuringTime(during_time);
    timer->m_cancle.store(false);
    std::unique_lock lock(this->m_mutex);
    this->m_timers.push(timer);
    lock.unlock();
    UpdateTimers();
}

void TimeEvent::UpdateTimers()
{
    struct timespec abstime;
    if (m_timers.empty())
    {
        abstime.tv_sec = 0;
        abstime.tv_nsec = 0;
    }
    else
    {
        std::shared_lock lock(this->m_mutex);
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

WaitEvent::WaitEvent(event::EventEngine* engine)
    :m_engine(engine), m_waitco(nullptr), m_event_in_engine(false)
{
}

NetWaitEvent::NetWaitEvent(event::EventEngine *engine, async::AsyncTcpSocket *socket)
    :WaitEvent(engine), m_socket(socket)
{
}

std::string NetWaitEvent::Name()
{
    switch (m_type)
    {
    case kWaitEventTypeSocket:
        return "SocketEvent";
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

bool NetWaitEvent::OnWaitPrepare(coroutine::Coroutine *co)
{
    switch (m_type)
    {
    case kWaitEventTypeSocket:
        return OnSocketPrepare(co);
    case kWaitEventTypeAccept:
        return OnAcceptPrepare(co);
    case kWaitEventTypeRecv:
        return OnRecvWaitPrepare(co);
    case kWaitEventTypeSend:
        return OnSendWaitPrepare(co);
    case kWaitEventTypeConnect:
        return OnConnectWaitPrepare(co);
    case kWaitEventTypeClose:
        return OnCloseWaitPrepare(co);
    default:
        break;
    }
    return false;
}

void NetWaitEvent::HandleEvent(EventEngine *engine)
{
    switch (m_type)
    {
    case kWaitEventTypeSocket:
        HandleSocketEvent(engine);
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
    }
}

int NetWaitEvent::GetEventType()
{
    switch (m_type)
    {
    case kWaitEventTypeSocket:
        break;
    case kWaitEventTypeAccept:
        return kEventTypeRead;
    case kWaitEventTypeRecv:
        return kEventTypeRead;
    case kWaitEventTypeSend:
        return kEventTypeWrite;
    case kWaitEventTypeConnect:
        return kEventTypeWrite;
    case kWaitEventTypeClose:
        return kEventTypeRead | kEventTypeWrite;
    default:
        break;
    }
    return kEventTypeNone;
}

GHandle NetWaitEvent::GetHandle()
{
    return m_socket->GetHandle();
}

void NetWaitEvent::Free(EventEngine *engine)
{
    if( m_event_in_engine ) engine->DelEvent(this);  
    delete this;
}

bool NetWaitEvent::OnSocketPrepare(coroutine::Coroutine *co)
{
    spdlog::debug("NetWaitEvent::OnSocketPrepare");
    m_socket->GetHandle().fd = socket(AF_INET, SOCK_STREAM, 0);
    galay::coroutine::Awaiter* awaiter = co->GetAwaiter();
    if( !awaiter ) {
        spdlog::error("NetWaitEvent::OnSendWaitPrepare awaiter is nullptr");
        return false;
    }
    if( m_socket->GetHandle().fd < 0 ) {
        m_socket->SetLastError("Socket failed");
        awaiter->SetResult(false);
    } else {
        awaiter->SetResult(true);
    }
    return false;
}

bool NetWaitEvent::OnAcceptPrepare(coroutine::Coroutine *co)
{
    sockaddr addr;
    socklen_t addrlen = sizeof(addr);
    GHandle handle {
        .fd = accept(m_socket->GetHandle().fd, &addr, &addrlen),
    };
    galay::coroutine::Awaiter* awaiter = co->GetAwaiter();
    if( !awaiter ) {
        spdlog::error("NetWaitEvent::OnSendWaitPrepare awaiter is nullptr");
        return false;
    }
    awaiter->SetResult(handle);
    return false;
}

bool NetWaitEvent::OnRecvWaitPrepare(coroutine::Coroutine *co)
{
    this->m_waitco = co;
    int recvBytes = DealRecv();
    spdlog::debug("NetWaitEvent::OnRecvWaitPrepare recvBytes: {}", recvBytes);
    if( recvBytes < 0 ) {
        if( errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR ){
            return true;
        }else {
            recvBytes = -1;
            std::string error = "recv error, ";
            error += strerror(errno);
            m_socket->SetLastError(error);
            return false;
        }
    }
    galay::coroutine::Awaiter* awaiter = co->GetAwaiter();
    if( awaiter ) awaiter->SetResult(recvBytes);
    else spdlog::error("NetWaitEvent::OnSendWaitPrepare awaiter is nullptr");
    return false;
}

bool NetWaitEvent::OnSendWaitPrepare(coroutine::Coroutine *co)
{
    this->m_waitco = co;
    int sendBytes = DealSend();
    spdlog::debug("NetWaitEvent::OnSendWaitPrepare sendBytes: {}", sendBytes);
    if( sendBytes < 0){
        if( errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR ){
        }else{
            galay::coroutine::Awaiter* awaiter = co->GetAwaiter();
            if( awaiter ) awaiter->SetResult(sendBytes);
            else spdlog::error("NetWaitEvent::OnSendWaitPrepare awaiter is nullptr");
            return false; 
        }
    }
    if (!m_socket->GetWBuffer().empty()){
        return true;
    }
    galay::coroutine::Awaiter* awaiter = co->GetAwaiter();
    if( awaiter ) awaiter->SetResult(sendBytes);
    else spdlog::error("NetWaitEvent::OnSendWaitPrepare awaiter is nullptr");
    return false;
}

bool NetWaitEvent::OnConnectWaitPrepare(coroutine::Coroutine *co)
{
    spdlog::debug("NetWaitEvent::OnConnectWaitPrepare");
    this->m_waitco = co;
    async::NetAddr addr = m_socket->GetConnectAddr();
    sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = inet_addr(addr.m_ip.c_str());
    saddr.sin_port = htons(addr.m_port);
    int ret = connect(m_socket->GetHandle().fd, (sockaddr*) &saddr, sizeof(sockaddr_in));
    if( ret != 0) {
        if( errno == EWOULDBLOCK || errno == EINTR || errno == EAGAIN || errno == EINPROGRESS) {
            return true;
        }
    }
    galay::coroutine::Awaiter* awaiter = m_waitco->GetAwaiter();
    if( ret == 0 ){
        if(awaiter) awaiter->SetResult(true);
        else spdlog::error("NetWaitEvent::OnConnectWaitPrepare awaiter is nullptr");
    } else {
        if(awaiter) awaiter->SetResult(false);
        else spdlog::error("NetWaitEvent::OnConnectWaitPrepare awaiter is nullptr");
    }
    return false;
}

bool NetWaitEvent::OnCloseWaitPrepare(coroutine::Coroutine *co)
{
    spdlog::debug("NetWaitEvent::OnCloseWaitPrepare");
    coroutine::Awaiter* awaiter = co->GetAwaiter(); 
    if (m_event_in_engine) {
        if( m_engine->DelEvent(this) != 0 ) {
            awaiter->SetResult(false);
            m_socket->SetLastError(strerror(errno));
            return false;
        }
    }
    int ret = closesocket(m_socket->GetHandle().fd);
    if( ret < 0 ) {
        m_socket->SetLastError(strerror(errno));
        awaiter->SetResult(false);
    } else {
        awaiter->SetResult(true);
    }
    return false;
}

void NetWaitEvent::HandleSocketEvent(EventEngine *engine)
{
    spdlog::debug("NetWaitEvent::HandleSocketEvent");
    // doing nothing
}

void NetWaitEvent::HandleAcceptEvent(EventEngine *engine)
{
    spdlog::debug("NetWaitEvent::HandleAcceptEvent");
    // doing nothing
}

void NetWaitEvent::HandleRecvEvent(EventEngine *engine)
{
    spdlog::debug("NetWaitEvent::HandleRecvEvent");
    int recvBytes = DealRecv();
    galay::coroutine::Awaiter* awaiter = m_waitco->GetAwaiter();
    if(awaiter) awaiter->SetResult(recvBytes);
    else spdlog::error("NetWaitEvent::HandleRecvEvent awaiter is nullptr");
    //2.唤醒协程
    scheduler::GetCoroutineScheduler(m_socket->GetHandle().fd % scheduler::GetCoroutineSchedulerNum())->ResumeCoroutine(m_waitco);
}

void NetWaitEvent::HandleSendEvent(EventEngine *engine)
{
    spdlog::debug("NetWaitEvent::HandleSendEvent");
    int sendBytes = DealSend();
    galay::coroutine::Awaiter* awaiter = m_waitco->GetAwaiter();
    if(awaiter) awaiter->SetResult(sendBytes);
    else spdlog::error("NetWaitEvent::HandleSendEvent awaiter is nullptr");
    scheduler::GetCoroutineScheduler(m_socket->GetHandle().fd % scheduler::GetCoroutineSchedulerNum())->ResumeCoroutine(m_waitco);
}

void NetWaitEvent::HandleConnectEvent(EventEngine *engine)
{
    spdlog::debug("NetWaitEvent::HandleConnectEvent");
    m_waitco->GetAwaiter()->SetResult(true);
    scheduler::GetCoroutineScheduler(m_socket->GetHandle().fd % scheduler::GetCoroutineSchedulerNum())->ResumeCoroutine(m_waitco);
}

void NetWaitEvent::HandleCloseEvent(EventEngine *engine)
{ 
    spdlog::debug("NetWaitEvent::HandleCloseEvent");
    //doing nothing
}

int NetWaitEvent::DealRecv()
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
            if (recvBytes == 0) recvBytes = -1;
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

int NetWaitEvent::DealSend()
{
    GHandle handle = this->m_socket->GetHandle();
    int sendBytes = 0;
    std::string_view wbuffer = m_socket->GetWBuffer();
    do {
        std::string_view view = wbuffer.substr(sendBytes);
        int ret = send(handle.fd, view.data(), view.size(), 0);
        if( ret == -1 ) {
            if(sendBytes == 0) {
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

}