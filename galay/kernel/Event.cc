#include "Event.h"
#include "Async.h"
#include "Scheduler.h"
#include "Session.hpp"
#include "Time.h"
#include "ExternApi.hpp"
#include <cstring>
#if defined(__linux__)
#include <sys/timerfd.h>
#elif  defined(WIN32) || defined(_WIN32) || defined(_WIN32_) || defined(WIN64) || defined(_WIN64) || defined(_WIN64_)

#endif

namespace galay::details
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
    if(details::EventEngine* t = m_engine.load(); !m_engine.compare_exchange_strong(t, engine)) {
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
    while (! m_timers.empty() && m_timers.top()->GetDeadline()  <= GetCurrentTimeMs() ) {
        auto timer = m_timers.top();
        m_timers.pop();
        timers.emplace_back(timer);
    }
    lock.unlock();
    for (auto timer: timers)
    {
        timer->Execute(weak_from_this());
    }
    UpdateTimers();
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    std::unique_lock lock(this->m_mutex);
    Timer::ptr timer = m_timers.top();
    m_timers.pop();
    lock.unlock();
    timer->Execute(weak_from_this());
#endif
}

bool TimeEvent::SetEventEngine(EventEngine *engine)
{
    details::EventEngine* t = m_engine.load();
    if(!m_engine.compare_exchange_strong(t, engine)) {
        return false;
    }
    return true;
}

EventEngine *TimeEvent::BelongEngine()
{
    return m_engine;
}

Timer::ptr TimeEvent::AddTimer(uint64_t timeout, std::function<void(std::weak_ptr<details::TimeEvent>, Timer::ptr)> &&func)
{
    auto timer = std::make_shared<Timer>(timeout, std::move(func));
    timer->ResetTimeout(timeout);
    timer->m_cancle.store(false);
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

void TimeEvent::AddTimer(const uint64_t timeout, const galay::Timer::ptr& timer)
{
    timer->ResetTimeout(timeout);
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
        uint64_t time = timer->GetRemainTime();
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

WaitEvent::WaitEvent()
    :m_engine(nullptr), m_waitco({})
{
}

bool WaitEvent::SetEventEngine(EventEngine *engine)
{
    if(details::EventEngine* t = m_engine.load(); !m_engine.compare_exchange_strong(t, engine)) {
        return false;
    }
    return true;
}

EventEngine* WaitEvent::BelongEngine()
{
    return m_engine.load();
}


NetWaitEvent::NetWaitEvent(AsyncNetIo *socket)
    :m_type(kWaitEventTypeError), m_socket(socket)
{
}

std::string NetWaitEvent::Name()
{
    switch (m_type)
    {
    case kWaitEventTypeError:
        return "ErrorEvent";
    case kTcpWaitEventTypeAccept:
        return "TcpAcceptEvent";
    case kTcpWaitEventTypeRecv:
        return "TcpRecvEvent";
    case kTcpWaitEventTypeSend:
        return "TcpSendEvent";
    case kTcpWaitEventTypeConnect:
        return "TcpConnectEvent";
    case kWaitEventTypeClose:
        return "CloseEvent";
    case kUdpWaitEventTypeRecvFrom:
        return "UdpRecvFromEvent";
    case kUdpWaitEventTypeSendTo:
        return "UdpSendToEvent";
    default:
        break;
    }
    return "UnknownEvent";
}

bool NetWaitEvent::OnWaitPrepare(Coroutine_wptr co, void* ctx)
{
    this->m_waitco = co;
    this->m_ctx = ctx;
    switch (m_type)
    {
    case kWaitEventTypeError:
        return false;
    case kTcpWaitEventTypeAccept:
        return OnTcpAcceptWaitPrepare(co, ctx);
    case kTcpWaitEventTypeRecv:
        return OnTcpRecvWaitPrepare(co, ctx);
    case kTcpWaitEventTypeSend:
        return OnTcpSendWaitPrepare(co, ctx);
    case kTcpWaitEventTypeConnect:
        return OnTcpConnectWaitPrepare(co, ctx);
    case kWaitEventTypeClose:
        return OnCloseWaitPrepare(co, ctx);
    case kUdpWaitEventTypeRecvFrom:
        return OnUdpRecvfromWaitPrepare(co, ctx);
    case kUdpWaitEventTypeSendTo:
        return OnUdpSendtoWaitPrepare(co, ctx);
    default:
        break;
    }
    return false;
}

void NetWaitEvent::HandleEvent(EventEngine *engine)
{
    switch (m_type)
    {
    case kWaitEventTypeError:
        HandleErrorEvent(engine);
        return;
    case kTcpWaitEventTypeAccept:
        HandleTcpAcceptEvent(engine);
        return;
    case kTcpWaitEventTypeRecv:
        HandleTcpRecvEvent(engine);
        return;
    case kTcpWaitEventTypeSend:
        HandleTcpSendEvent(engine);
        return;
    case kTcpWaitEventTypeConnect:
        HandleTcpConnectEvent(engine);
        return;
    case kWaitEventTypeClose:
        HandleCloseEvent(engine);
        return;
    case kUdpWaitEventTypeRecvFrom:
        HandleUdpRecvfromEvent(engine);
        return;
    case kUdpWaitEventTypeSendTo:
        HandleUdpSendtoEvent(engine);
        return;
    default: 
        return;
    }
}

EventType NetWaitEvent::GetEventType()
{
    switch (m_type)
    {
    case kWaitEventTypeError:
        return kEventTypeError;
    case kTcpWaitEventTypeAccept:
    case kTcpWaitEventTypeRecv:
        return kEventTypeRead;
    case kTcpWaitEventTypeSend:
    case kTcpWaitEventTypeConnect:
        return kEventTypeWrite;
    case kWaitEventTypeClose:
        return kEventTypeNone;
    case kUdpWaitEventTypeRecvFrom:
        return kEventTypeRead;
    case kUdpWaitEventTypeSendTo:
        return kEventTypeWrite;
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

bool NetWaitEvent::OnTcpAcceptWaitPrepare(const Coroutine_wptr co, void *ctx)
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
    auto awaiter = static_cast<galay::Awaiter<GHandle>*>(co.lock()->GetAwaiter());
    if( handle.fd < 0 ) {
        if( errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR ) {
            
        }else{
            m_socket->GetErrorCode() = error::MakeErrorCode(error::Error_AcceptError, errno);
        }
    }
    awaiter->SetResult(handle);
    return false;
}

bool NetWaitEvent::OnTcpRecvWaitPrepare(const Coroutine_wptr co, void* ctx)
{
    auto* iov = static_cast<TcpIOVec*>(ctx);
    int recvBytes = TcpDealRecv(iov);
    if( recvBytes == eCommonNonBlocking ) {
        return true;
    }
    auto awaiter = static_cast<galay::Awaiter<int>*>(co.lock()->GetAwaiter());
    awaiter->SetResult(recvBytes);
    return false;
}

bool NetWaitEvent::OnTcpSendWaitPrepare(const Coroutine_wptr co, void* ctx)
{
    auto* iov = static_cast<TcpIOVec*>(ctx);
    int sendBytes = TcpDealSend(iov);
    if( sendBytes == eCommonNonBlocking){
        return true;
    }
    auto awaiter = static_cast<galay::Awaiter<int>*>(co.lock()->GetAwaiter());
    awaiter->SetResult(sendBytes);
    return false;
}

bool NetWaitEvent::OnTcpConnectWaitPrepare(Coroutine_wptr co, void* ctx)
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
    auto awaiter = static_cast<galay::Awaiter<bool>*>(co.lock()->GetAwaiter());
    if( ret == 0 ){
        awaiter->SetResult(true);
    } else {
        m_socket->GetErrorCode() = error::MakeErrorCode(error::Error_ConnectError, errno);
        awaiter->SetResult(false);
    }
    return false;
}

bool NetWaitEvent::OnCloseWaitPrepare(const Coroutine_wptr co, void* ctx)
{
    auto awaiter = static_cast<galay::Awaiter<bool>*>(co.lock()->GetAwaiter()); 
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

bool NetWaitEvent::OnUdpRecvfromWaitPrepare(const Coroutine_wptr co, void *ctx)
{
    auto iov = static_cast<UdpIOVec*>(ctx);
    int recvBytes = UdpDealRecvfrom(iov);
    if( recvBytes == eCommonNonBlocking ) {
        return true;
    }
    auto awaiter = static_cast<galay::Awaiter<int>*>(co.lock()->GetAwaiter());
    awaiter->SetResult(recvBytes);
    return false;
}

bool NetWaitEvent::OnUdpSendtoWaitPrepare(const Coroutine_wptr co, void *ctx)
{
    auto iov = static_cast<UdpIOVec*>(ctx);
    int sendBytes = UdpDealSendto(iov);
    if( sendBytes == eCommonNonBlocking){
        return true;
    }
    auto awaiter = static_cast<galay::Awaiter<int>*>(co.lock()->GetAwaiter());
    awaiter->SetResult(sendBytes);
    return false;
}

void NetWaitEvent::HandleErrorEvent(EventEngine *engine)
{
}

void NetWaitEvent::HandleTcpAcceptEvent(EventEngine *engine)
{
}

void NetWaitEvent::HandleTcpRecvEvent(EventEngine *engine)
{
    int recvBytes = TcpDealRecv(static_cast<TcpIOVec*>(m_ctx));
    auto awaiter = static_cast<galay::Awaiter<int>*>(m_waitco.lock()->GetAwaiter());
    awaiter->SetResult(recvBytes);
    //2.唤醒协程
    m_waitco.lock()->BelongScheduler()->ToResumeCoroutine(m_waitco);
}

void NetWaitEvent::HandleTcpSendEvent(EventEngine *engine)
{
    int sendBytes = TcpDealSend(static_cast<TcpIOVec*>(m_ctx));
    auto awaiter = static_cast<galay::Awaiter<int>*>(m_waitco.lock()->GetAwaiter());
    awaiter->SetResult(sendBytes);
    m_waitco.lock()->BelongScheduler()->ToResumeCoroutine(m_waitco);
}

void NetWaitEvent::HandleTcpConnectEvent(EventEngine *engine)
{
    auto awaiter = static_cast<galay::Awaiter<bool>*>(m_waitco.lock()->GetAwaiter());
    awaiter->SetResult(true);
    m_waitco.lock()->BelongScheduler()->ToResumeCoroutine(m_waitco);
}

void NetWaitEvent::HandleCloseEvent(EventEngine *engine)
{ 
    //doing nothing
}

void NetWaitEvent::HandleUdpRecvfromEvent(EventEngine *engine)
{
    int recvBytes = UdpDealRecvfrom(static_cast<UdpIOVec*>(m_ctx));
    auto awaiter = static_cast<galay::Awaiter<int>*>(m_waitco.lock()->GetAwaiter());
    awaiter->SetResult(recvBytes);
    //2.唤醒协程
    m_waitco.lock()->BelongScheduler()->ToResumeCoroutine(m_waitco);
}

void NetWaitEvent::HandleUdpSendtoEvent(EventEngine *engine)
{
    int sendBytes = UdpDealSendto(static_cast<UdpIOVec*>(m_ctx));
    auto awaiter = static_cast<galay::Awaiter<int>*>(m_waitco.lock()->GetAwaiter());
    awaiter->SetResult(sendBytes);
    m_waitco.lock()->BelongScheduler()->ToResumeCoroutine(m_waitco);
}

int NetWaitEvent::TcpDealRecv(TcpIOVec* iov)
{
    auto [fd] = this->m_socket->GetHandle();
    size_t recvBytes = iov->m_offset + iov->m_length > iov->m_size ? iov->m_size - iov->m_offset : iov->m_length;
    int length = recv(fd, iov->m_buffer + iov->m_offset, recvBytes, 0);
    if( length == -1 ) {
        if( errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR ) {
            m_socket->GetErrorCode() = error::MakeErrorCode(error::Error_RecvError, errno);
            return eCommonOtherFailed;
        } else {
            return eCommonNonBlocking;
        }
    }
    LogTrace("[Recv, Handle: {}, Byte: {}\nData: {}]", fd, length, std::string_view(iov->m_buffer + iov->m_offset, length));
    iov->m_offset += length;
    return length;
}

int NetWaitEvent::TcpDealSend(TcpIOVec* iov)
{
    auto [fd] = this->m_socket->GetHandle();
    size_t writeBytes = iov->m_offset + iov->m_length > iov->m_size ? iov->m_size - iov->m_offset : iov->m_length;
    const int length = send(fd, iov->m_buffer + iov->m_offset, writeBytes, 0);
    if( length == -1 ) {
        if( errno != EAGAIN && errno != EWOULDBLOCK && errno == EINTR ) {
            m_socket->GetErrorCode() = error::MakeErrorCode(error::Error_SendError, errno);
            return eCommonOtherFailed;
        } else {
            return eCommonNonBlocking;
        }
    }
    LogTrace("[Send, Handle: {}, Byte: {}\nData: {}]", fd, length, std::string_view(iov->m_buffer + iov->m_offset, length));
    iov->m_offset += length;
    return length;
}

int NetWaitEvent::UdpDealRecvfrom(UdpIOVec *iov)
{
    auto [fd] = this->m_socket->GetHandle();
    size_t recvBytes = iov->m_offset + iov->m_length > iov->m_size ? iov->m_size - iov->m_offset : iov->m_length;
    sockaddr addr;
    socklen_t addrlen = sizeof(addr);
    int length = recvfrom(fd, iov->m_buffer + iov->m_offset, recvBytes, 0, &addr, &addrlen);
    if( length == -1 ) {
        if( errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR ) {
            m_socket->GetErrorCode() = error::MakeErrorCode(error::Error_RecvError, errno);
            return eCommonOtherFailed;
        } else {
            return eCommonNonBlocking;
        }
    }
    LogTrace("[Recv, Handle: {}, Byte: {}\nData: {}]", fd, length, std::string_view(iov->m_buffer + iov->m_offset, length));
    iov->m_offset += length;
    iov->m_addr.m_ip = std::string(inet_ntoa(reinterpret_cast<sockaddr_in*>(&addr)->sin_addr));
    iov->m_addr.m_ip = ntohs(reinterpret_cast<sockaddr_in*>(&addr)->sin_port);
    return length;
}

int NetWaitEvent::UdpDealSendto(UdpIOVec *iov)
{
    auto [fd] = this->m_socket->GetHandle();
    size_t writeBytes = iov->m_offset + iov->m_length > iov->m_size ? iov->m_size - iov->m_offset : iov->m_length;
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(iov->m_addr.m_ip.c_str());
    addr.sin_port = htons(iov->m_addr.m_port);
    int length = sendto(fd, iov->m_buffer + iov->m_offset, writeBytes, 0, reinterpret_cast<sockaddr*>(&addr), sizeof(sockaddr));
    if( length == -1 ) {
        if( errno != EAGAIN && errno != EWOULDBLOCK && errno == EINTR ) {
            m_socket->GetErrorCode() = error::MakeErrorCode(error::Error_SendError, errno);
            return eCommonOtherFailed;
        } else {
            spdlog::info("{}", strerror(errno));
            return eCommonNonBlocking;
        }
    }
    LogTrace("[Send, Handle: {}, Byte: {}\nData: {}]", fd, length, std::string_view(iov->m_buffer + iov->m_offset, length));
    iov->m_offset += length;
    return length;
}

NetSslWaitEvent::NetSslWaitEvent(AsyncSslNetIo *socket)
    :NetWaitEvent(socket)
{
}

std::string NetSslWaitEvent::Name()
{
    switch (m_type)
    {
    case kWaitEventTypeError:
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
    case kWaitEventTypeClose:
        return "CloseEvent";
    case kWaitEventTypeSslClose:
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
    case kWaitEventTypeError:
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
    case kWaitEventTypeClose:
    case kWaitEventTypeSslClose:
        return kEventTypeNone;
    default:
        break;
    }
    return kEventTypeNone;
}

bool NetSslWaitEvent::OnWaitPrepare(Coroutine_wptr co, void* ctx)
{
    this->m_waitco = co;
    this->m_ctx = ctx;
    switch (m_type)
    {
    case kWaitEventTypeError:
        return false;
    case kTcpWaitEventTypeAccept:
        return OnTcpAcceptWaitPrepare(co, ctx);
    case kTcpWaitEventTypeSslAccept:
        return OnTcpSslAcceptWaitPrepare(co, ctx);
    case kTcpWaitEventTypeRecv:
        return OnTcpRecvWaitPrepare(co, ctx);
    case kTcpWaitEventTypeSslRecv:
        return OnTcpSslRecvWaitPrepare(co, ctx);
    case kTcpWaitEventTypeSend:
        return OnTcpSendWaitPrepare(co, ctx);
    case kTcpWaitEventTypeSslSend:
        return OnTcpSslSendWaitPrepare(co, ctx);
    case kTcpWaitEventTypeConnect:
        return OnTcpConnectWaitPrepare(co, ctx);
    case kTcpWaitEventTypeSslConnect:
        return OnTcpSslConnectWaitPrepare(co, ctx);
    case kWaitEventTypeClose:
        return OnCloseWaitPrepare(co, ctx);
    case kWaitEventTypeSslClose:
        return OnTcpSslCloseWaitPrepare(co, ctx);
    default:
        break;
    }
    return false;
}

void NetSslWaitEvent::HandleEvent(EventEngine *engine)
{
    switch (m_type)
    {
    case kWaitEventTypeError:
        HandleErrorEvent(engine);
        return;
    case kTcpWaitEventTypeAccept:
        HandleTcpAcceptEvent(engine);
        return;
    case kTcpWaitEventTypeSslAccept:
        HandleTcpSslAcceptEvent(engine);
        return;
    case kTcpWaitEventTypeRecv:
        HandleTcpRecvEvent(engine);
        return;
    case kTcpWaitEventTypeSslRecv:
        HandleTcpSslRecvEvent(engine);
        return;
    case kTcpWaitEventTypeSend:
        HandleTcpSendEvent(engine);
        return;
    case kTcpWaitEventTypeSslSend:
        HandleTcpSslSendEvent(engine);
        return;
    case kTcpWaitEventTypeConnect:
        HandleTcpConnectEvent(engine);
        return;
    case kTcpWaitEventTypeSslConnect:
        HandleTcpSslConnectEvent(engine);
        return;
    case kWaitEventTypeClose:
        HandleCloseEvent(engine);
        return;
    case kWaitEventTypeSslClose:
        HandleTcpSslCloseEvent(engine);
        return;
    default:
        return;
    }
}

AsyncSslNetIo *NetSslWaitEvent::GetAsyncTcpSocket() const
{
    return dynamic_cast<AsyncSslNetIo*>(m_socket);
}

NetSslWaitEvent::~NetSslWaitEvent()
= default;

bool NetSslWaitEvent::OnTcpSslAcceptWaitPrepare(const Coroutine_wptr co, void* ctx)
{
    auto awaiter = static_cast<galay::Awaiter<bool>*>(co.lock()->GetAwaiter());
    SSL* ssl = dynamic_cast<AsyncSslNetIo*>(m_socket)->GetSSL();
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

bool NetSslWaitEvent::OnTcpSslConnectWaitPrepare(const Coroutine_wptr co, void* ctx)
{
    
    auto awaiter = static_cast<galay::Awaiter<bool>*>(co.lock()->GetAwaiter());
    SSL* ssl = dynamic_cast<AsyncSslNetIo*>(m_socket)->GetSSL();
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

bool NetSslWaitEvent::OnTcpSslRecvWaitPrepare(const Coroutine_wptr co, void* ctx)
{
    
    int recvBytes = TcpDealRecv(static_cast<TcpIOVec*>(ctx));
    auto awaiter = static_cast<galay::Awaiter<int>*>(co.lock()->GetAwaiter());
    if(recvBytes == eCommonNonBlocking) {
        return true;
    }
    awaiter->SetResult(recvBytes);
    return false;
}

bool NetSslWaitEvent::OnTcpSslSendWaitPrepare(const Coroutine_wptr co, void* ctx)
{
    
    auto* iov = static_cast<TcpIOVec*>(m_ctx);
    int sendBytes = TcpDealSend(iov);
    auto awaiter = static_cast<galay::Awaiter<int>*>(co.lock()->GetAwaiter());
    if( sendBytes == eCommonNonBlocking){
        return true;
    }
    awaiter->SetResult(sendBytes);
    return false;
}

bool NetSslWaitEvent::OnTcpSslCloseWaitPrepare(const Coroutine_wptr co, void* ctx)
{
    auto awaiter = static_cast<galay::Awaiter<bool>*>(co.lock()->GetAwaiter()); 
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
    SSL*& ssl = dynamic_cast<AsyncSslNetIo*>(m_socket)->GetSSL();
    SSL_shutdown(ssl);
    SSL_free(ssl);
    ssl = nullptr;
    return false;
}

void NetSslWaitEvent::HandleTcpSslAcceptEvent(EventEngine *engine)
{
    auto awaiter = static_cast<galay::Awaiter<bool>*>(m_waitco.lock()->GetAwaiter());
    SSL* ssl = dynamic_cast<AsyncSslNetIo*>(m_socket)->GetSSL();
    const int r = SSL_do_handshake(ssl);
    LogTrace("[SSL_do_handshake, handle: {}]", m_socket->GetHandle().fd);
    if( r == 1 ){
        awaiter->SetResult(true);
        m_waitco.lock()->BelongScheduler()->ToResumeCoroutine(m_waitco);
        return;
    }
    m_ssl_error = SSL_get_error(ssl, r);
    if( m_ssl_error == SSL_ERROR_WANT_READ || m_ssl_error == SSL_ERROR_WANT_WRITE ){
        engine->ModEvent(this, nullptr);
    } else {
        awaiter->SetResult(false);
        m_socket->GetErrorCode() = error::MakeErrorCode(error::Error_SSLAcceptError, errno);
        m_waitco.lock()->BelongScheduler()->ToResumeCoroutine(m_waitco);
    }
    
}

void NetSslWaitEvent::HandleTcpSslConnectEvent(EventEngine *engine)
{
    auto awaiter = static_cast<galay::Awaiter<bool>*>(m_waitco.lock()->GetAwaiter());
    SSL* ssl = dynamic_cast<AsyncSslNetIo*>(m_socket)->GetSSL();
    const int r = SSL_do_handshake(ssl);
    LogTrace("[SSL_do_handshake, handle: {}]", m_socket->GetHandle().fd);
    if( r == 1 ){
        awaiter->SetResult(true);
        m_waitco.lock()->BelongScheduler()->ToResumeCoroutine(m_waitco);
    }
    m_ssl_error = SSL_get_error(ssl, r);
    if( m_ssl_error == SSL_ERROR_WANT_READ || m_ssl_error == SSL_ERROR_WANT_WRITE ){
        engine->ModEvent(this, nullptr);
    } else {
        awaiter->SetResult(false);
        m_socket->GetErrorCode() = error::MakeErrorCode(error::Error_SSLConnectError, errno);
        m_waitco.lock()->BelongScheduler()->ToResumeCoroutine(m_waitco);
    }
    
}

void NetSslWaitEvent::HandleTcpSslRecvEvent(EventEngine *engine)
{
    int recvBytes = TcpDealRecv(static_cast<TcpIOVec*>(m_ctx));
    auto awaiter = static_cast<galay::Awaiter<int>*>(m_waitco.lock()->GetAwaiter());
    awaiter->SetResult(recvBytes);
    //2.唤醒协程
    m_waitco.lock()->BelongScheduler()->ToResumeCoroutine(m_waitco);
}

void NetSslWaitEvent::HandleTcpSslSendEvent(EventEngine *engine)
{
    int sendBytes = TcpDealSend(static_cast<TcpIOVec*>(m_ctx));
    auto awaiter = static_cast<galay::Awaiter<int>*>(m_waitco.lock()->GetAwaiter());
    awaiter->SetResult(sendBytes);
    m_waitco.lock()->BelongScheduler()->ToResumeCoroutine(m_waitco);
}

void NetSslWaitEvent::HandleTcpSslCloseEvent(EventEngine *engine)
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

int NetSslWaitEvent::TcpDealRecv(TcpIOVec* iov)
{
    SSL* ssl = GetAsyncTcpSocket()->GetSSL();
    size_t recvBytes = iov->m_offset + iov->m_length > iov->m_size ? iov->m_size - iov->m_offset : iov->m_length;
    int length = SSL_read(ssl, iov->m_buffer + iov->m_offset, recvBytes);
    if( length == -1 ) {
        if( errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR ) {
            m_socket->GetErrorCode() = error::MakeErrorCode(error::Error_RecvError, errno);
            return eCommonOtherFailed;
        } else {
            return eCommonNonBlocking;
        }
    }
    LogTrace("[Recv, Handle: {}, Byte: {}\nData: {}]", GetAsyncTcpSocket()->GetHandle().fd, length, std::string_view(iov->m_buffer + iov->m_offset, length));
    iov->m_offset += length;
    return length;
}

int NetSslWaitEvent::TcpDealSend(TcpIOVec* iov)
{
    SSL* ssl = GetAsyncTcpSocket()->GetSSL();
    size_t writeBytes = iov->m_offset + iov->m_length > iov->m_size ? iov->m_size - iov->m_offset : iov->m_length;
    const int length = SSL_write(ssl, iov->m_buffer + iov->m_offset, writeBytes);
    if( length == -1 ) {
        if( errno != EAGAIN && errno != EWOULDBLOCK && errno == EINTR ) {
            m_socket->GetErrorCode() = error::MakeErrorCode(error::Error_SendError, errno);
            return eCommonOtherFailed;
        } else {
            return eCommonNonBlocking;
        }
    }
    LogTrace("[Send, Handle: {}, Byte: {}\nData: {}]", GetAsyncTcpSocket()->GetHandle().fd, length, std::string_view(iov->m_buffer + iov->m_offset, length));
    iov->m_offset += length;
    return length;
}
//#endif

FileIoWaitEvent::FileIoWaitEvent(AsyncFileIo *fileio)
    :m_fileio(fileio), m_type(kFileIoWaitEventTypeError)
{
    
}

std::string FileIoWaitEvent::Name()
{
    switch (m_type)
    {
#ifdef __linux__
    case kFileIoWaitEventTypeLinuxAio:
        return "LinuxAioEvent";
#endif
    case kFileIoWaitEventTypeRead:
        return "KReadEvent";
    case kFileIoWaitEventTypeWrite:
        return "KWriteEvent";
    case kFileIoWaitEventTypeClose:
        return "CloseEvent";
    default:
        break;
    }
    return "Unknown";
}

bool FileIoWaitEvent::OnWaitPrepare(Coroutine_wptr co, void *ctx)
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
    case kFileIoWaitEventTypeClose:
        return OnCloseWaitPrepare(co, ctx);
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
    case kFileIoWaitEventTypeClose:
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
    case kFileIoWaitEventTypeClose:
        return kEventTypeNone;
    default: ;
    }
    return kEventTypeNone;
}

GHandle FileIoWaitEvent::GetHandle()
{
    switch (m_type)
    {
#ifdef __linux__
    case kFileIoWaitEventTypeLinuxAio:
        return static_cast<AsyncFileNativeAio*>(m_fileio)->GetEventHandle();
#endif
    default:
        break;
    }
    return m_fileio->GetHandle();
}

FileIoWaitEvent::~FileIoWaitEvent()
{
    if(m_engine) {
        m_engine.load()->DelEvent(this, nullptr);
    }
}

#ifdef __linux__
bool FileIoWaitEvent::OnAioWaitPrepare(Coroutine_wptr co, void *ctx)
{
    return true;
}


void FileIoWaitEvent::HandleAioEvent(EventEngine* engine)
{
    uint64_t finish_nums = 0;
    auto fileio = static_cast<AsyncFileNativeAio*>(m_fileio);
    int ret = read(fileio->GetEventHandle().fd, &finish_nums, sizeof(uint64_t));
    io_event events[DEFAULT_IO_EVENTS_SIZE] = {0};
    int r = io_getevents(fileio->GetIoContext(), 1, DEFAULT_IO_EVENTS_SIZE, events, nullptr);
    LogTrace("[io_getevents return {} events]", r);
    while (r --> 0)
    {
        auto& event = events[r];
        if(event.data != nullptr) {
            static_cast<AioCallback*>(event.data)->OnAioComplete(&event);
        }
    }
    if( fileio->IoFinished(finish_nums) ){
        engine->DelEvent(this, nullptr);
        m_waitco.lock()->BelongScheduler()->ToResumeCoroutine(m_waitco);
    }
}
#endif

bool FileIoWaitEvent::OnKReadWaitPrepare(const Coroutine_wptr co, void *ctx)
{
    auto* iov = static_cast<FileIOVec*>(ctx);
    size_t readBytes = iov->m_offset + iov->m_length > iov->m_size ? iov->m_size - iov->m_offset : iov->m_length;
    int length = read(m_fileio->GetHandle().fd, iov->m_buffer + iov->m_offset, readBytes);
    if(length < 0){
        if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
            return true;
        } else {
            m_fileio->GetErrorCode() = error::MakeErrorCode(error::Error_FileReadError, errno);
            length = eCommonOtherFailed;
        }        
    }
    LogTrace("[Handle: {}, ReadBytes: {}]", m_fileio->GetHandle().fd, length);
    static_cast<Awaiter<int>*>(co.lock()->GetAwaiter())->SetResult(length);
    iov->m_offset += length;
    return false;
}

void FileIoWaitEvent::HandleKReadEvent(EventEngine *engine)
{
    auto* iov = static_cast<FileIOVec*>(m_ctx);
    size_t readBytes = iov->m_offset + iov->m_length > iov->m_size ? iov->m_size - iov->m_offset : iov->m_length;
    int length = read(m_fileio->GetHandle().fd, iov->m_buffer + iov->m_offset, readBytes);
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
    static_cast<Awaiter<int>*>(m_waitco.lock()->GetAwaiter())->SetResult(length);
    iov->m_offset += length;
    m_waitco.lock()->BelongScheduler()->ToResumeCoroutine(m_waitco);
}

bool FileIoWaitEvent::OnKWriteWaitPrepare(Coroutine_wptr co, void *ctx)
{
    auto* iov = static_cast<FileIOVec*>(ctx);
    size_t writeBytes = iov->m_offset + iov->m_length > iov->m_size ? iov->m_size - iov->m_offset : iov->m_length;
    int length = write(m_fileio->GetHandle().fd, iov->m_buffer + iov->m_offset, writeBytes);
    if(length < 0){
         if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
            return true;
        }else {
            m_fileio->GetErrorCode() = error::MakeErrorCode(error::Error_FileReadError, errno);
            length = eCommonOtherFailed;
        }
    }
    LogTrace("[Handle: {}, WriteBytes: {}]", m_fileio->GetHandle().fd, length);
    iov->m_offset += length;
    static_cast<Awaiter<int>*>(co.lock()->GetAwaiter())->SetResult(length);
    return false;
}

void FileIoWaitEvent::HandleKWriteEvent(EventEngine *engine)
{
    auto* iov = static_cast<FileIOVec*>(m_ctx);
    size_t writeBytes = iov->m_offset + iov->m_length > iov->m_size ? iov->m_size - iov->m_offset : iov->m_length;
    int length = write(m_fileio->GetHandle().fd, iov->m_buffer + iov->m_offset, writeBytes);
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
    static_cast<Awaiter<int>*>(m_waitco.lock()->GetAwaiter())->SetResult(length);
    iov->m_offset += length;
    m_waitco.lock()->BelongScheduler()->ToResumeCoroutine(m_waitco);
}

bool FileIoWaitEvent::OnCloseWaitPrepare(const Coroutine_wptr co, void *ctx)
{
    auto awaiter = static_cast<Awaiter<bool>*>(co.lock()->GetAwaiter()); 
    if (m_engine) {
        if( m_engine.load()->DelEvent(this, nullptr) != 0 ) {
            awaiter->SetResult(false);
            m_fileio->GetErrorCode() = error::MakeErrorCode(error::Error_CloseError, errno);
            return false;
        }
    }
    LogTrace("[Close {}]", m_fileio->GetHandle().fd);
    if(const int ret = close(m_fileio->GetHandle().fd); ret < 0 ) {
        m_fileio->GetErrorCode() = error::MakeErrorCode(error::Error_CloseError, errno);
        awaiter->SetResult(false);
    } else {
        awaiter->SetResult(true);
    }
    return false;
}
}