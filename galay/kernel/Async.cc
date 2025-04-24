#include "Async.hpp"
#include "galay/utils/System.h"
#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <fcntl.h>
    #include <sys/sendfile.h>
#ifdef __linux__
    #include <sys/eventfd.h>
#endif
#elif defined(WIN32) || defined(_WIN32) || defined(_WIN32_) || defined(WIN64) || defined(_WIN64) || defined(_WIN64_)
    #include <ws2ipdef.h>
    #include <WS2tcpip.h>
#endif

namespace galay
{


HandleOption::HandleOption(const GHandle handle)
    : m_handle(handle), m_error_code(0)
{
    
}

bool HandleOption::HandleBlock()
{
#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    int flag = fcntl(m_handle.fd, F_GETFL, 0);
    flag &= ~O_NONBLOCK;
    int ret = fcntl(m_handle.fd, F_SETFL, flag);
#elif defined(WIN32) || defined(_WIN32) || defined(_WIN32_) || defined(WIN64) || defined(_WIN64) || defined(_WIN64_)
    u_long mode = 0; // 1 表示非阻塞模式
    int ret = ioctlsocket(m_handle, FIONBIO, &mode);
#endif
    if (ret < 0) {
        m_error_code = error::MakeErrorCode(error::ErrorCode::Error_SetBlockError, errno);
        return false;
    }
    return true;
}

bool HandleOption::HandleNonBlock()
{
#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    int flag = fcntl(m_handle.fd, F_GETFL, 0);
    flag |= O_NONBLOCK;
    int ret = fcntl(m_handle.fd, F_SETFL, flag);
#elif defined(WIN32) || defined(_WIN32) || defined(_WIN32_) || defined(WIN64) || defined(_WIN64) || defined(_WIN64_)
    u_long mode = 1; // 1 表示非阻塞模式
    int ret = ioctlsocket(m_handle.fd, FIONBIO, &mode);
#endif
    if (ret < 0) {
        m_error_code = error::MakeErrorCode(error::ErrorCode::Error_SetNoBlockError, errno);
        return false;
    }
    return true;
}

bool HandleOption::HandleReuseAddr()
{
#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    int option = 1;
    int ret = setsockopt(m_handle.fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
#elif  defined(WIN32) || defined(_WIN32) || defined(_WIN32_) || defined(WIN64) || defined(_WIN64) || defined(_WIN64_)
    BOOL option = TRUE;
    int ret = setsockopt(m_handle.fd, SOL_SOCKET, SO_REUSEADDR, (char*)&option, sizeof(option));
#endif
    if (ret < 0) {   
        m_error_code = error::MakeErrorCode(error::ErrorCode::Error_SetSockOptError, errno);
        return false;
    }
    return true;
}

bool HandleOption::HandleReusePort()
{
#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    constexpr int option = 1;
    if (const int ret = setsockopt(m_handle.fd, SOL_SOCKET, SO_REUSEPORT, &option, sizeof(option)); ret < 0) {
        m_error_code = error::MakeErrorCode(error::ErrorCode::Error_SetSockOptError, errno);
        return false;
    }
#elif  defined(WIN32) || defined(_WIN32) || defined(_WIN32_) || defined(WIN64) || defined(_WIN64) || defined(_WIN64_)
    //To Do
#endif
    return true;
}

uint32_t& HandleOption::GetErrorCode()
{
    return m_error_code;
}

}

namespace galay::details
{
Resumer::Resumer(int64_t& timeout)
    :m_timeout(timeout)
{
}

void Resumer::StartTimeout(int64_t timeout)
{
    if(!m_timer) {
        m_timer = std::make_shared<Timer>([resumer = shared_from_this()](time_event_wptr event, Timer::ptr timer) {
            Resumer::DoingAsyncTimeout(resumer, event, timer);
        });
    }
    if(m_waitco.expired()) {
        LogError("Resumer::StartTimeout error, waitco is expired");
        return;
    }
    if(m_timer->IsCancel()) {
        m_lastest_end_time = utils::GetCurrentTimeMs() + timeout;
        m_waitco.lock()->GetEventScheduler()->AddTimer(m_timer, timeout);
    }
    else {
        m_lastest_end_time = utils::GetCurrentTimeMs() + timeout;
    }
}

void Resumer::Reset(CoroutineBase::wptr waitco)
{
    m_resumed.store(false);
    m_waitco = waitco;
    m_lastest_end_time = 0;
}

void Resumer::Reset(CoroutineBase::wptr waitco, Timer::ptr timer)
{
    m_resumed.store(false);
    m_timer = timer;
    m_waitco = waitco;
    m_lastest_end_time = 0;
}

bool Resumer::IsResumed()
{
    return m_resumed.load();
}

bool Resumer::Resume()
{
    bool resumed = false;
    if(m_resumed.compare_exchange_strong(resumed, true)) {
        m_waitco.lock()->GetCoScheduler()->ToResumeCoroutine(m_waitco);
        return true;
    } 
    return false;
}

void Resumer::SetResumeInterfaceType(ResumeInterfaceType type)
{
    m_type = type;
}

CoroutineBase::wptr Resumer::GetWaitCo()
{
    return m_waitco;
}

int64_t Resumer::GetLastestEndTime()
{
    return m_lastest_end_time;
}

void Resumer::DoingAsyncTimeout(Resumer::ptr resumer, time_event_wptr event, Timer::ptr timer)
{
    int64_t now = utils::GetCurrentTimeMs();
    if (resumer->GetLastestEndTime() <= now) {
        if(!resumer->IsResumed()) {
            if(resumer->m_waitco.expired()) return;
            resumer->m_timeout = -1;
            switch (resumer->m_type)
            {
            case kResumeInterfaceType_Accept:
                static_cast<Awaiter<GHandle>*>(resumer->m_waitco.lock()->GetAwaiter())->SetResult({-1});
                break;
            case kResumeInterfaceType_Connect:
                static_cast<Awaiter<bool>*>(resumer->m_waitco.lock()->GetAwaiter())->SetResult(false);
                break;
            case kResumeInterfaceType_Recv:
                static_cast<Awaiter<int>*>(resumer->m_waitco.lock()->GetAwaiter())->SetResult(-2);
                break;
            case kResumeInterfaceType_Send:
                static_cast<Awaiter<int>*>(resumer->m_waitco.lock()->GetAwaiter())->SetResult(-2);
                break;
            case kResumeInterfaceType_SendFile:
                static_cast<Awaiter<bool>*>(resumer->m_waitco.lock()->GetAwaiter())->SetResult(false);
                break;
            case kResumeInterfaceType_RecvFrom:
                static_cast<Awaiter<int>*>(resumer->m_waitco.lock()->GetAwaiter())->SetResult(-2);
                break;
            case kResumeInterfaceType_SendTo:
                static_cast<Awaiter<int>*>(resumer->m_waitco.lock()->GetAwaiter())->SetResult(-2);
                break;
            case kResumeInterfaceType_SSLAccept:
                static_cast<Awaiter<bool>*>(resumer->m_waitco.lock()->GetAwaiter())->SetResult(false);
                break;
            case kResumeInterfaceType_SSLConnect:
                static_cast<Awaiter<bool>*>(resumer->m_waitco.lock()->GetAwaiter())->SetResult(false);
                break;
            case kResumeInterfaceType_SSLRead:
                static_cast<Awaiter<int>*>(resumer->m_waitco.lock()->GetAwaiter())->SetResult(-2);
                break;
            case kResumeInterfaceType_SSLWrite:
                static_cast<Awaiter<int>*>(resumer->m_waitco.lock()->GetAwaiter())->SetResult(-2);
                break;
            case kResumeInterfaceType_FileRead:
                static_cast<Awaiter<int>*>(resumer->m_waitco.lock()->GetAwaiter())->SetResult(-2);
                break;
            case kResumeInterfaceType_FileWrite:
                static_cast<Awaiter<int>*>(resumer->m_waitco.lock()->GetAwaiter())->SetResult(-2);
                break;
            default:
                break;
            }
            resumer->m_timer->SetIsCancel(true);
            resumer->Resume();
        }
    } else {
        event.lock()->AddTimer(timer, resumer->GetLastestEndTime() - now);
    }
}

NetWaitEvent::NetWaitEvent(AsyncNetEventContext* context)
    :m_type(kWaitEventTypeError), m_async_context(context)
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
    case kTcpWaitEventTypeSendfile:
        return "TcpSendfileEvent";
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

bool NetWaitEvent::OnWaitPrepare(CoroutineBase::wptr co, void* ctx)
{
    this->m_async_context->m_resumer->Reset(co);
    this->m_ctx = ctx;
    bool res = false;
    switch (m_type)
    {
    case kWaitEventTypeError:
        return false;
    case kTcpWaitEventTypeAccept:
        res = OnTcpAcceptWaitPrepare(co, ctx);
        break;
    case kTcpWaitEventTypeRecv:
        res = OnTcpRecvWaitPrepare(co, ctx);
        break;
    case kTcpWaitEventTypeSend:
        res = OnTcpSendWaitPrepare(co, ctx);
        break;
    case kTcpWaitEventTypeSendfile:
        res = OnTcpSendfileWaitPrepare(co, ctx);
        break;
    case kTcpWaitEventTypeConnect:
        res = OnTcpConnectWaitPrepare(co, ctx);
        break;
    case kWaitEventTypeClose:
        res = OnCloseWaitPrepare(co, ctx);
        break;
    case kUdpWaitEventTypeRecvFrom:
        res = OnUdpRecvfromWaitPrepare(co, ctx);
        break;
    case kUdpWaitEventTypeSendTo:
        res = OnUdpSendtoWaitPrepare(co, ctx);
        break;
    default:
        break;
    }
    if(res) {
        if(this->m_async_context->m_timeout != -1) {
            this->m_async_context->m_resumer->StartTimeout(this->m_async_context->m_timeout);
        }
    }
    return res;
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
    case kTcpWaitEventTypeSendfile:
        HandleTcpSendfileEvent(engine);
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
    case kTcpWaitEventTypeSendfile:
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
    return m_async_context->m_handle;
}

NetWaitEvent::~NetWaitEvent()
{
    if(this->m_engine) this->m_engine.load()->DelEvent(this, nullptr);
}

bool NetWaitEvent::OnTcpAcceptWaitPrepare(const CoroutineBase::wptr co, void *ctx)
{
    THost* netaddr = static_cast<THost*>(ctx);
    sockaddr addr{};
    socklen_t addrlen = sizeof(addr);
    GHandle handle {
        .fd = accept(m_async_context->m_handle.fd, &addr, &addrlen),
    };
    netaddr->m_ip = inet_ntoa(((sockaddr_in*)&addr)->sin_addr);
    netaddr->m_port = ntohs(((sockaddr_in*)&addr)->sin_port);
    LogTrace("[Accept Address: {}:{}]", netaddr->m_ip, netaddr->m_port);
    auto awaiter = static_cast<Awaiter<GHandle>*>(co.lock()->GetAwaiter());
    if( handle.fd < 0 ) {
        if( errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR ) {
            awaiter->SetResult(std::move(handle));
            return false;
        }else{
            m_async_context->m_error_code = error::MakeErrorCode(error::Error_AcceptError, errno);
        }
    }
    awaiter->SetResult(std::move(handle));
    return false;
}

bool NetWaitEvent::OnTcpRecvWaitPrepare(const CoroutineBase::wptr co, void* ctx)
{
    auto* iov = static_cast<TcpIOVec*>(ctx);
    int recvBytes = TcpDealRecv(iov);
    if( recvBytes == eCommonNonBlocking ) {
        return true;
    }
    auto awaiter = static_cast<Awaiter<int>*>(co.lock()->GetAwaiter());
    awaiter->SetResult(std::move(recvBytes));
    return false;
}

bool NetWaitEvent::OnTcpSendWaitPrepare(const CoroutineBase::wptr co, void* ctx)
{
    auto* iov = static_cast<TcpIOVec*>(ctx);
    int sendBytes = TcpDealSend(iov);
    if( sendBytes == eCommonNonBlocking){
        return true;
    }
    auto awaiter = static_cast<Awaiter<int>*>(co.lock()->GetAwaiter());
    awaiter->SetResult(std::move(sendBytes));
    return false;
}

bool NetWaitEvent::OnTcpSendfileWaitPrepare(const CoroutineBase::wptr co, void *ctx)
{
    auto* desc = static_cast<FileDesc*>(ctx);
    int sendBytes = TcpDealSendfile(desc);
    if( sendBytes == eCommonNonBlocking){
        return true;
    }
    auto awaiter = static_cast<Awaiter<bool>*>(co.lock()->GetAwaiter());
    awaiter->SetResult(std::move(sendBytes));
    return false;
}

bool NetWaitEvent::OnTcpConnectWaitPrepare(CoroutineBase::wptr co, void* ctx)
{
    auto* addr = static_cast<THost*>(ctx);
    LogTrace("[Connect to {}:{}]", addr->m_ip, addr->m_port);
    sockaddr_in saddr{};
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = inet_addr(addr->m_ip.c_str());
    saddr.sin_port = htons(addr->m_port);
    const int ret = connect(m_async_context->m_handle.fd, reinterpret_cast<sockaddr*>(&saddr), sizeof(sockaddr_in));
    if( ret != 0) {
        if( errno == EWOULDBLOCK || errno == EINTR || errno == EAGAIN || errno == EINPROGRESS) {
            return true;
        }
    }
    auto awaiter = static_cast<Awaiter<bool>*>(co.lock()->GetAwaiter());
    if( ret == 0 ){
        awaiter->SetResult(true);
        m_async_context->m_is_connected = true;
    } else {
        m_async_context->m_error_code = error::MakeErrorCode(error::Error_ConnectError, errno);
        awaiter->SetResult(false);
    }
    return false;
}


bool NetWaitEvent::OnCloseWaitPrepare(const CoroutineBase::wptr co, void* ctx)
{
    auto awaiter = static_cast<Awaiter<bool>*>(co.lock()->GetAwaiter()); 
    if (this->m_engine) {
        if( this->m_engine.load()->DelEvent(this, nullptr) != 0 ) {
            awaiter->SetResult(false);
            m_async_context->m_error_code = error::MakeErrorCode(error::Error_CloseError, errno);
            return false;
        }
    }
    LogTrace("[Close {}]", m_async_context->m_handle.fd);
    if(const int ret = close(m_async_context->m_handle.fd); ret < 0 ) {
        m_async_context->m_error_code = error::MakeErrorCode(error::Error_CloseError, errno);
        awaiter->SetResult(false);
    } else {
        m_async_context->m_is_connected = false;
        awaiter->SetResult(true);
    }
    return false;
}


bool NetWaitEvent::OnUdpRecvfromWaitPrepare(const CoroutineBase::wptr co, void *ctx)
{
    auto iov = static_cast<UdpIOVec*>(ctx);
    int recvBytes = UdpDealRecvfrom(iov);
    if( recvBytes == eCommonNonBlocking ) {
        return true;
    }
    auto awaiter = static_cast<Awaiter<int>*>(co.lock()->GetAwaiter());
    awaiter->SetResult(std::move(recvBytes));
    return false;
}


bool NetWaitEvent::OnUdpSendtoWaitPrepare(const CoroutineBase::wptr co, void *ctx)
{
    auto iov = static_cast<UdpIOVec*>(ctx);
    int sendBytes = UdpDealSendto(iov);
    if( sendBytes == eCommonNonBlocking){
        return true;
    }
    auto awaiter = static_cast<Awaiter<int>*>(co.lock()->GetAwaiter());
    awaiter->SetResult(std::move(sendBytes));
    return false;
}


void NetWaitEvent::HandleErrorEvent(EventEngine *engine)
{
}


void NetWaitEvent::HandleTcpAcceptEvent(EventEngine *engine)
{
    THost* netaddr = static_cast<THost*>(m_ctx);
    sockaddr addr{};
    socklen_t addrlen = sizeof(addr);
    GHandle handle {
        .fd = accept(m_async_context->m_handle.fd, &addr, &addrlen),
    };
    netaddr->m_ip = inet_ntoa(((sockaddr_in*)&addr)->sin_addr);
    netaddr->m_port = ntohs(((sockaddr_in*)&addr)->sin_port);
    LogTrace("[Accept Address: {}:{}]", netaddr->m_ip, netaddr->m_port);
    auto awaiter = static_cast<Awaiter<GHandle>*>(m_async_context->m_resumer->GetWaitCo().lock()->GetAwaiter());
    if( handle.fd < 0 ) {
        if( errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR ) {
        }else{
            m_async_context->m_error_code = error::MakeErrorCode(error::Error_AcceptError, errno);
        }
    }
    awaiter->SetResult(std::move(handle));
    m_async_context->Resume();
}


void NetWaitEvent::HandleTcpRecvEvent(EventEngine *engine)
{
    int recvBytes = TcpDealRecv(static_cast<TcpIOVec*>(m_ctx));
    auto awaiter = static_cast<Awaiter<int>*>(this->m_async_context->m_resumer->GetWaitCo().lock()->GetAwaiter());
    awaiter->SetResult(std::move(recvBytes));
    //2.唤醒协程
    this->m_async_context->Resume();
}


void NetWaitEvent::HandleTcpSendEvent(EventEngine *engine)
{
    int sendBytes = TcpDealSend(static_cast<TcpIOVec*>(m_ctx));
    auto awaiter = static_cast<Awaiter<int>*>(this->m_async_context->m_resumer->GetWaitCo().lock()->GetAwaiter());
    awaiter->SetResult(std::move(sendBytes));
    this->m_async_context->Resume();
}

void NetWaitEvent::HandleTcpSendfileEvent(EventEngine *engine)
{
    auto* desc = static_cast<FileDesc*>(m_ctx);
    int sendBytes = TcpDealSendfile(desc);
    auto awaiter = static_cast<Awaiter<int>*>(this->m_async_context->m_resumer->GetWaitCo().lock()->GetAwaiter());
    awaiter->SetResult(std::move(sendBytes));
    m_async_context->Resume();
}

void NetWaitEvent::HandleTcpConnectEvent(EventEngine *engine)
{
    auto awaiter = static_cast<Awaiter<bool>*>(this->m_async_context->m_resumer->GetWaitCo().lock()->GetAwaiter());
    awaiter->SetResult(true);
    m_async_context->m_is_connected = true;
    this->m_async_context->Resume();
}


void NetWaitEvent::HandleCloseEvent(EventEngine *engine)
{ 
    //doing nothing
}


void NetWaitEvent::HandleUdpRecvfromEvent(EventEngine *engine)
{
    int recvBytes = UdpDealRecvfrom(static_cast<UdpIOVec*>(m_ctx));
    auto awaiter = static_cast<Awaiter<int>*>(this->m_async_context->m_resumer->GetWaitCo().lock()->GetAwaiter());
    awaiter->SetResult(std::move(recvBytes));
    //2.唤醒协程
    this->m_async_context->Resume();
}


void NetWaitEvent::HandleUdpSendtoEvent(EventEngine *engine)
{
    int sendBytes = UdpDealSendto(static_cast<UdpIOVec*>(this->m_ctx));
    auto awaiter = static_cast<Awaiter<int>*>(this->m_async_context->m_resumer->GetWaitCo().lock()->GetAwaiter());
    awaiter->SetResult(std::move(sendBytes));
    this->m_async_context->Resume();
}


int NetWaitEvent::TcpDealRecv(TcpIOVec* iov)
{
    auto [fd] = this->m_async_context->m_handle;
    size_t recvBytes = iov->m_offset + iov->m_length > iov->m_size ? iov->m_size - iov->m_offset : iov->m_length;
    int length = recv(fd, iov->m_buffer + iov->m_offset, recvBytes, 0);
    if( length == -1 ) {
        if( errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR ) {
            this->m_async_context->m_error_code = error::MakeErrorCode(error::Error_RecvError, errno);
            return eCommonDisConnect;
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
    auto [fd] = this->m_async_context->m_handle;
    size_t writeBytes = iov->m_offset + iov->m_length > iov->m_size ? iov->m_size - iov->m_offset : iov->m_length;
    const int length = send(fd, iov->m_buffer + iov->m_offset, writeBytes, 0);
    if( length == -1 ) {
        if( errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR ) {
            this->m_async_context->m_error_code = error::MakeErrorCode(error::Error_SendError, errno);
            return eCommonDisConnect;
        } else {
            return eCommonNonBlocking;
        }
    }
    LogTrace("[Send, Handle: {}, Byte: {}\nData: {}]", fd, length, std::string_view(iov->m_buffer + iov->m_offset, length));
    iov->m_offset += length;
    return length;
}

#ifdef __linux__
int NetWaitEvent::TcpDealSendfile(FileDesc* desc)
{
    int length = sendfile(this->m_async_context->m_handle.fd, desc->m_file_handle.fd, &desc->m_offset, desc->m_file_size - desc->m_offset);
    if(length == -1) {
        if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
            return eCommonNonBlocking;
        } else {
            this->m_async_context->m_error_code = error::MakeErrorCode(error::Error_SendError, errno);
            return eCommonDisConnect;
        }
    }
    LogTrace("[SendFile, From File Handle: {} To Handle: {}, Byte: {}]", this->m_async_context->m_handle.fd, desc->m_file_handle.fd, length);
    return length;
}

#endif

int NetWaitEvent::UdpDealRecvfrom(UdpIOVec *iov)
{
    auto [fd] = this->m_async_context->m_handle;
    size_t recvBytes = iov->m_offset + iov->m_length > iov->m_size ? iov->m_size - iov->m_offset : iov->m_length;
    sockaddr addr;
    socklen_t addrlen = sizeof(addr);
    int length = recvfrom(fd, iov->m_buffer + iov->m_offset, recvBytes, 0, &addr, &addrlen);
    if( length == -1 ) {
        if( errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR ) {
            this->m_async_context->m_error_code = error::MakeErrorCode(error::Error_RecvError, errno);
            return eCommonUnknown;
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
    auto [fd] = this->m_async_context->m_handle;
    size_t writeBytes = iov->m_offset + iov->m_length > iov->m_size ? iov->m_size - iov->m_offset : iov->m_length;
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(iov->m_addr.m_ip.c_str());
    addr.sin_port = htons(iov->m_addr.m_port);
    int length = sendto(fd, iov->m_buffer + iov->m_offset, writeBytes, 0, reinterpret_cast<sockaddr*>(&addr), sizeof(sockaddr));
    if( length == -1 ) {
        if( errno != EAGAIN && errno != EWOULDBLOCK && errno == EINTR ) {
            this->m_async_context->m_error_code = error::MakeErrorCode(error::Error_SendError, errno);
            return eCommonUnknown;
        } else {
            spdlog::info("{}", strerror(errno));
            return eCommonNonBlocking;
        }
    }
    LogTrace("[Send, Handle: {}, Byte: {}\nData: {}]", fd, length, std::string_view(iov->m_buffer + iov->m_offset, length));
    iov->m_offset += length;
    return length;
}


NetSslWaitEvent::NetSslWaitEvent(AsyncSslNetEventContext* async_context)
    :NetWaitEvent(async_context)
{
}


std::string NetSslWaitEvent::Name()
{
    switch (this->m_type)
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
    case kTcpWaitEventTypeSendfile:
        return "SendfileEvent";
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
    switch (this->m_type)
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
    case kTcpWaitEventTypeSendfile:
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


bool NetSslWaitEvent::OnWaitPrepare(CoroutineBase::wptr co, void* ctx)
{
    this->m_async_context->m_resumer->Reset(co);
    this->m_ctx = ctx;
    bool res = false;
    switch (this->m_type)
    {
    case kWaitEventTypeError:
        return false;
    case kTcpWaitEventTypeAccept:
        res = OnTcpAcceptWaitPrepare(co, ctx);
        break;
    case kTcpWaitEventTypeSslAccept:
        res = OnTcpSslAcceptWaitPrepare(co, ctx);
        break;
    case kTcpWaitEventTypeRecv:
        res = OnTcpRecvWaitPrepare(co, ctx);
        break;
    case kTcpWaitEventTypeSslRecv:
        res = OnTcpSslRecvWaitPrepare(co, ctx);
        break;
    case kTcpWaitEventTypeSend:
        res = OnTcpSendWaitPrepare(co, ctx);
        break;
    case kTcpWaitEventTypeSendfile:
        res = OnTcpSendfileWaitPrepare(co, ctx);
        break;
    case kTcpWaitEventTypeSslSend:
        res = OnTcpSslSendWaitPrepare(co, ctx);
        break;
    case kTcpWaitEventTypeConnect:
        res = OnTcpConnectWaitPrepare(co, ctx);
        break;
    case kTcpWaitEventTypeSslConnect:
        res = OnTcpSslConnectWaitPrepare(co, ctx);
        break;
    case kWaitEventTypeClose:
        res = OnCloseWaitPrepare(co, ctx);
        break;
    case kWaitEventTypeSslClose:
        res = OnTcpSslCloseWaitPrepare(co, ctx);
        break;
    default:
        break;
    }
    if(res) {
        if(this->m_async_context->m_timeout != -1) {
            this->m_async_context->m_resumer->StartTimeout(this->m_async_context->m_timeout);
        }
    }
    return res;
}


void NetSslWaitEvent::HandleEvent(EventEngine *engine)
{
    switch (this->m_type)
    {
    case kWaitEventTypeError:
        this->HandleErrorEvent(engine);
        return;
    case kTcpWaitEventTypeAccept:
        this->HandleTcpAcceptEvent(engine);
        return;
    case kTcpWaitEventTypeSslAccept:
        this->HandleTcpSslAcceptEvent(engine);
        return;
    case kTcpWaitEventTypeRecv:
        this->HandleTcpRecvEvent(engine);
        return;
    case kTcpWaitEventTypeSslRecv:
        this->HandleTcpSslRecvEvent(engine);
        return;
    case kTcpWaitEventTypeSend:
        this->HandleTcpSendEvent(engine);
        return;
    case kTcpWaitEventTypeSendfile:
        this->HandleTcpSendfileEvent(engine);
        return;
    case kTcpWaitEventTypeSslSend:
        this->HandleTcpSslSendEvent(engine);
        return;
    case kTcpWaitEventTypeConnect:
        this->HandleTcpConnectEvent(engine);
        return;
    case kTcpWaitEventTypeSslConnect:
        this->HandleTcpSslConnectEvent(engine);
        return;
    case kWaitEventTypeClose:
        this->HandleCloseEvent(engine);
        return;
    case kWaitEventTypeSslClose:
        this->HandleTcpSslCloseEvent(engine);
        return;
    default:
        return;
    }
}

AsyncSslNetEventContext* NetSslWaitEvent::GetAsyncContext() const
{
    return static_cast<AsyncSslNetEventContext*>(m_async_context);
}


bool NetSslWaitEvent::OnTcpSslAcceptWaitPrepare(const CoroutineBase::wptr co, void* ctx)
{
    auto awaiter = static_cast<Awaiter<bool>*>(co.lock()->GetAwaiter());
    SSL* ssl = static_cast<AsyncSslNetEventContext*>(this->m_async_context)->m_ssl;
    SSL_set_accept_state(ssl);
    int r = SSL_do_handshake(ssl);
    LogTrace("[SSL_do_handshake, handle: {}]", this->m_async_context->m_handle.fd);
    if( r == 1 ){
        awaiter->SetResult(true);
        return false;
    }
    this->m_ssl_error = SSL_get_error(ssl, r);
    if( this->m_ssl_error == SSL_ERROR_WANT_READ || this->m_ssl_error == SSL_ERROR_WANT_WRITE ){
        return true;
    }
    awaiter->SetResult(false);
    this->m_async_context->m_error_code = error::MakeErrorCode(error::Error_SSLAcceptError, errno);
    return false;
}


bool NetSslWaitEvent::OnTcpSslConnectWaitPrepare(const CoroutineBase::wptr co, void* ctx)
{
    
    auto awaiter = static_cast<Awaiter<bool>*>(co.lock()->GetAwaiter());
    SSL* ssl = static_cast<AsyncSslNetEventContext*>(this->m_async_context)->m_ssl;
    SSL_set_connect_state(ssl);
    int r = SSL_do_handshake(ssl);
    LogTrace("[SSL_do_handshake, handle: {}]", this->m_async_context->m_handle.fd);
    if( r == 1 ){
        awaiter->SetResult(true);
        static_cast<AsyncSslNetEventContext*>(this->m_async_context)->m_is_connected = true;
        return false;
    }
    this->m_ssl_error = SSL_get_error(ssl, r);
    if( this->m_ssl_error == SSL_ERROR_WANT_READ || this->m_ssl_error == SSL_ERROR_WANT_WRITE ){
        return true;
    }
    awaiter->SetResult(false);
    this->m_async_context->m_error_code = error::MakeErrorCode(error::Error_SSLConnectError, errno);
    return false;
}


bool NetSslWaitEvent::OnTcpSslRecvWaitPrepare(const CoroutineBase::wptr co, void* ctx)
{
    
    int recvBytes = TcpDealRecv(static_cast<TcpIOVec*>(ctx));
    auto awaiter = static_cast<Awaiter<int>*>(co.lock()->GetAwaiter());
    if(recvBytes == eCommonNonBlocking) {
        return true;
    }
    awaiter->SetResult(std::move(recvBytes));
    return false;
}


bool NetSslWaitEvent::OnTcpSslSendWaitPrepare(const CoroutineBase::wptr co, void* ctx)
{
    
    auto* iov = static_cast<TcpIOVec*>(this->m_ctx);
    int sendBytes = TcpDealSend(iov);
    auto awaiter = static_cast<Awaiter<int>*>(co.lock()->GetAwaiter());
    if( sendBytes == eCommonNonBlocking){
        return true;
    }
    awaiter->SetResult(std::move(sendBytes));
    return false;
}


bool NetSslWaitEvent::OnTcpSslCloseWaitPrepare(const CoroutineBase::wptr co, void* ctx)
{
    auto awaiter = static_cast<Awaiter<bool>*>(co.lock()->GetAwaiter()); 
    if (this->m_engine) {
        if( this->m_engine.load()->DelEvent(this, nullptr) != 0 ) {
            awaiter->SetResult(false);
            this->m_async_context->m_error_code = error::MakeErrorCode(error::Error_DelEventError, errno);
            return false;
        }
    }
    LogTrace("[Close {}]", this->m_async_context->m_handle.fd);
    int ret = close(this->m_async_context->m_handle.fd);
    if( ret < 0 ) {
        this->m_async_context->m_error_code = error::MakeErrorCode(error::Error_SSLCloseError, errno);
        awaiter->SetResult(false);
        return false;
    } else {
        m_async_context->m_is_connected = false;
        awaiter->SetResult(true);
    }
    SSL*& ssl = static_cast<AsyncSslNetEventContext*>(this->m_async_context)->m_ssl;
    SSL_shutdown(ssl);
    SSL_free(ssl);
    ssl = nullptr;
    return false;
}


void NetSslWaitEvent::HandleTcpSslAcceptEvent(EventEngine *engine)
{
    auto awaiter = static_cast<Awaiter<bool>*>(this->m_async_context->m_resumer->GetWaitCo().lock()->GetAwaiter());
    SSL* ssl = dynamic_cast<AsyncSslNetEventContext*>(this->m_async_context)->m_ssl;
    const int r = SSL_do_handshake(ssl);
    LogTrace("[SSL_do_handshake, handle: {}]", this->m_async_context->m_handle.fd);
    if( r == 1 ){
        awaiter->SetResult(true);
        this->m_async_context->Resume();
        return;
    }
    this->m_ssl_error = SSL_get_error(ssl, r);
    if( this->m_ssl_error == SSL_ERROR_WANT_READ || this->m_ssl_error == SSL_ERROR_WANT_WRITE ){
        engine->ModEvent(this, nullptr);
    } else {
        awaiter->SetResult(false);
        this->m_async_context->m_error_code = error::MakeErrorCode(error::Error_SSLAcceptError, errno);
        this->m_async_context->Resume();
    }
    
}


void NetSslWaitEvent::HandleTcpSslConnectEvent(EventEngine *engine)
{
    auto awaiter = static_cast<Awaiter<bool>*>(this->m_async_context->m_resumer->GetWaitCo().lock()->GetAwaiter());
    SSL* ssl = static_cast<AsyncSslNetEventContext*>(this->m_async_context)->m_ssl;
    const int r = SSL_do_handshake(ssl);
    LogTrace("[SSL_do_handshake, handle: {}]", this->m_async_context->m_handle.fd);
    if( r == 1 ){
        awaiter->SetResult(true);
        this->m_async_context->m_is_connected = true;
        this->m_async_context->Resume();
    }
    this->m_ssl_error = SSL_get_error(ssl, r);
    if( this->m_ssl_error == SSL_ERROR_WANT_READ || this->m_ssl_error == SSL_ERROR_WANT_WRITE ){
        engine->ModEvent(this, nullptr);
    } else {
        awaiter->SetResult(false);
        this->m_async_context->m_error_code = error::MakeErrorCode(error::Error_SSLConnectError, errno);
        this->m_async_context->Resume();
    }
    
}


void NetSslWaitEvent::HandleTcpSslRecvEvent(EventEngine *engine)
{
    int recvBytes = TcpDealRecv(static_cast<TcpIOVec*>(this->m_ctx));
    auto awaiter = static_cast<Awaiter<int>*>(this->m_async_context->m_resumer->GetWaitCo().lock()->GetAwaiter());
    awaiter->SetResult(std::move(recvBytes));
    //2.唤醒协程
    this->m_async_context->Resume();
}


void NetSslWaitEvent::HandleTcpSslSendEvent(EventEngine *engine)
{
    int sendBytes = TcpDealSend(static_cast<TcpIOVec*>(this->m_ctx));
    auto awaiter = static_cast<Awaiter<int>*>(this->m_async_context->m_resumer->GetWaitCo().lock()->GetAwaiter());
    awaiter->SetResult(std::move(sendBytes));
    this->m_async_context->Resume();
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
    SSL* ssl = GetAsyncContext()->m_ssl;
    size_t recvBytes = iov->m_offset + iov->m_length > iov->m_size ? iov->m_size - iov->m_offset : iov->m_length;
    int length = SSL_read(ssl, iov->m_buffer + iov->m_offset, recvBytes);
    if( length == -1 ) {
        if( errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR ) {
            this->m_async_context->m_error_code = error::MakeErrorCode(error::Error_RecvError, errno);
            return eCommonUnknown;
        } else {
            return eCommonNonBlocking;
        }
    }
    LogTrace("[Recv, Handle: {}, Byte: {}\nData: {}]", GetAsyncContext()->m_handle.fd, length, std::string_view(iov->m_buffer + iov->m_offset, length));
    iov->m_offset += length;
    return length;
}


int NetSslWaitEvent::TcpDealSend(TcpIOVec* iov)
{
    SSL* ssl = GetAsyncContext()->m_ssl;
    size_t writeBytes = iov->m_offset + iov->m_length > iov->m_size ? iov->m_size - iov->m_offset : iov->m_length;
    const int length = SSL_write(ssl, iov->m_buffer + iov->m_offset, writeBytes);
    if( length == -1 ) {
        if( errno != EAGAIN && errno != EWOULDBLOCK && errno == EINTR ) {
            this->m_async_context->m_error_code = error::MakeErrorCode(error::Error_SendError, errno);
            return eCommonUnknown;
        } else {
            return eCommonNonBlocking;
        }
    }
    LogTrace("[Send, Handle: {}, Byte: {}\nData: {}]", GetAsyncContext()->m_handle.fd, length, std::string_view(iov->m_buffer + iov->m_offset, length));
    iov->m_offset += length;
    return length;
}
//#endif


FileIoWaitEvent::FileIoWaitEvent(AsyncFileEventContext* async_context)
    :m_async_context(async_context), m_type(kFileIoWaitEventTypeError)
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


bool FileIoWaitEvent::OnWaitPrepare(CoroutineBase::wptr co, void *ctx)
{
    this->m_async_context->m_resumer->Reset(co);
    this->m_ctx = ctx;
    bool res = false;
    switch (m_type)
    {
#ifdef __linux__
    case kFileIoWaitEventTypeLinuxAio:
        res = OnAioWaitPrepare(co, ctx);
        break;
#endif
    case kFileIoWaitEventTypeRead:
        res = OnKReadWaitPrepare(co, ctx);
        break;
    case kFileIoWaitEventTypeWrite:
        res = OnKWriteWaitPrepare(co, ctx);
        break;
    case kFileIoWaitEventTypeClose:
        res = OnCloseWaitPrepare(co, ctx);
        break;
    default:
        break;
    }
    if(res) {
        if(this->m_async_context->m_timeout != -1) {
            this->m_async_context->m_resumer->StartTimeout(this->m_async_context->m_timeout);
        }
    }
    return res;
}


void FileIoWaitEvent::HandleEvent(EventEngine *engine)
{
    switch (m_type)
    {
#ifdef __linux__
    case kFileIoWaitEventTypeLinuxAio:
        this->HandleAioEvent(engine);
        break;
#endif
    case kFileIoWaitEventTypeRead:
        this->HandleKReadEvent(engine);
        break;
    case kFileIoWaitEventTypeWrite:
        this->HandleKWriteEvent(engine);
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
        return static_cast<AsyncLinuxFileEventContext*>(this->m_async_context)->m_event_handle;
#endif
    default:
        break;
    }
    return this->m_async_context->m_handle;
}


FileIoWaitEvent::~FileIoWaitEvent()
{
    if(this->m_engine) {
        this->m_engine.load()->DelEvent(this, nullptr);
    }
}

#ifdef __linux__

bool FileIoWaitEvent::OnAioWaitPrepare(CoroutineBase::wptr co, void *ctx)
{
    return true;
}


void FileIoWaitEvent::HandleAioEvent(EventEngine* engine)
{
    uint64_t finish_nums = 0;
    auto async_context = static_cast<AsyncLinuxFileEventContext*>(this->m_async_context);
    int ret = read(async_context->m_event_handle.fd, &finish_nums, sizeof(uint64_t));
    io_event events[DEFAULT_LINUX_AIO_EVENTS_SIZE] = {0};
    int r = io_getevents(async_context->m_ioctx, 1, DEFAULT_LINUX_AIO_EVENTS_SIZE, events, nullptr);
    LogTrace("[io_getevents return {} events]", r);
    while (r --> 0)
    {
        auto& event = events[r];
        if(event.data != nullptr) {
            static_cast<AioCallback*>(event.data)->OnAioComplete(&event);
        }
    }
    if( async_context->IoFinished(finish_nums) ){
        engine->DelEvent(this, nullptr);
        this->m_async_context->m_resumer->Resume();
    }
}
#endif


bool FileIoWaitEvent::OnKReadWaitPrepare(const CoroutineBase::wptr co, void *ctx)
{
    auto* iov = static_cast<FileIOVec*>(ctx);
    size_t readBytes = iov->m_offset + iov->m_length > iov->m_size ? iov->m_size - iov->m_offset : iov->m_length;
    int length = read(this->m_async_context->m_handle.fd, iov->m_buffer + iov->m_offset, readBytes);
    if(length < 0){
        if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
            return true;
        } else {
            this->m_async_context->m_error_code = error::MakeErrorCode(error::Error_FileReadError, errno);
            length = eCommonUnknown;
        }        
    }
    LogTrace("[Handle: {}, ReadBytes: {}]", this->m_async_context->m_handle.fd, length);
    iov->m_offset += length;
    static_cast<Awaiter<int>*>(co.lock()->GetAwaiter())->SetResult(std::move(length));
    return false;
}


void FileIoWaitEvent::HandleKReadEvent(EventEngine *engine)
{
    auto* iov = static_cast<FileIOVec*>(m_ctx);
    size_t readBytes = iov->m_offset + iov->m_length > iov->m_size ? iov->m_size - iov->m_offset : iov->m_length;
    int length = read(this->m_async_context->m_handle.fd, iov->m_buffer + iov->m_offset, readBytes);
    if( length < 0 ) {
        if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
            length = eCommonNonBlocking;
        } else {
            this->m_async_context->m_error_code = error::MakeErrorCode(error::Error_FileReadError, errno);
            length = eCommonUnknown;
        }
    }
    LogTrace("[Handle: {}, ReadBytes: {}]", this->m_async_context->m_handle.fd, length);
    engine->DelEvent(this, nullptr);
    iov->m_offset += length;
    static_cast<Awaiter<int>*>(this->m_async_context->m_resumer->GetWaitCo().lock()->GetAwaiter())->SetResult(std::move(length));
    this->m_async_context->Resume();
}


bool FileIoWaitEvent::OnKWriteWaitPrepare(CoroutineBase::wptr co, void *ctx)
{
    auto* iov = static_cast<FileIOVec*>(ctx);
    size_t writeBytes = iov->m_offset + iov->m_length > iov->m_size ? iov->m_size - iov->m_offset : iov->m_length;
    int length = write(this->m_async_context->m_handle.fd, iov->m_buffer + iov->m_offset, writeBytes);
    if(length < 0){
         if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
            return true;
        }else {
            this->m_async_context->m_error_code = error::MakeErrorCode(error::Error_FileReadError, errno);
            length = eCommonUnknown;
        }
    }
    LogTrace("[Handle: {}, WriteBytes: {}]", this->m_async_context->m_handle.fd, length);
    iov->m_offset += length;
    static_cast<Awaiter<int>*>(co.lock()->GetAwaiter())->SetResult(std::move(length));
    return false;
}


void FileIoWaitEvent::HandleKWriteEvent(EventEngine *engine)
{
    auto* iov = static_cast<FileIOVec*>(m_ctx);
    size_t writeBytes = iov->m_offset + iov->m_length > iov->m_size ? iov->m_size - iov->m_offset : iov->m_length;
    int length = write(this->m_async_context->m_handle.fd, iov->m_buffer + iov->m_offset, writeBytes);
    if(length < 0){
         if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
            length = eCommonNonBlocking;
        }else {
            this->m_async_context->m_error_code = error::MakeErrorCode(error::Error_FileReadError, errno);
            length = eCommonUnknown;
        }
    }
    LogTrace("[Handle: {}, WriteBytes: {}]", this->m_async_context->m_handle.fd, length);
    engine->DelEvent(this, nullptr);
    iov->m_offset += length;
    static_cast<Awaiter<int>*>(this->m_async_context->m_resumer->GetWaitCo().lock()->GetAwaiter())->SetResult(std::move(length));
    this->m_async_context->Resume();
}


bool FileIoWaitEvent::OnCloseWaitPrepare(const CoroutineBase::wptr co, void *ctx)
{
    auto awaiter = static_cast<Awaiter<bool>*>(co.lock()->GetAwaiter()); 
    if (this->m_engine) {
        if( this->m_engine.load()->DelEvent(this, nullptr) != 0 ) {
            awaiter->SetResult(false);
            this->m_async_context->m_error_code = error::MakeErrorCode(error::Error_CloseError, errno);
            return false;
        }
    }
    LogTrace("[Close {}]", this->m_async_context->m_handle.fd);
    if(const int ret = close(this->m_async_context->m_handle.fd); ret < 0 ) {
        this->m_async_context->m_error_code = error::MakeErrorCode(error::Error_CloseError, errno);
        awaiter->SetResult(false);
    } else {
        awaiter->SetResult(true);
    }
    return false;
}



}