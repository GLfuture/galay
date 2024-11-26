#include "Async.h"
#include "Event.h"
#include "EventEngine.h"
#include "WaitAction.h"
#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <fcntl.h>
#ifdef __linux__
    #include <sys/eventfd.h>
#endif
#elif defined(WIN32) || defined(_WIN32) || defined(_WIN32_) || defined(WIN64) || defined(_WIN64) || defined(_WIN64_)
    #include <ws2ipdef.h>
    #include <WS2tcpip.h>
#endif
#include <string.h>

namespace galay::async
{

HandleOption::HandleOption(GHandle handle)
{
    this->m_handle = handle;
}

bool HandleOption::HandleBlock()
{
#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    int flag = fcntl(this->m_handle.fd, F_GETFL, 0);
    flag &= ~O_NONBLOCK;
    int ret = fcntl(this->m_handle.fd, F_SETFL, flag);
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
    int flag = fcntl(this->m_handle.fd, F_GETFL, 0);
    flag |= O_NONBLOCK;
    int ret = fcntl(this->m_handle.fd, F_SETFL, flag);
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
    int ret = setsockopt(this->m_handle.fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
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
    int option = 1;
    int ret = setsockopt(this->m_handle.fd, SOL_SOCKET, SO_REUSEPORT, &option, sizeof(option));
    if (ret < 0) {
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

bool AsyncSocket(AsyncNetIo *asocket)
{
    asocket->GetErrorCode() = error::ErrorCode::Error_NoError;
    asocket->GetHandle().fd = socket(AF_INET, SOCK_STREAM, 0);
    if (asocket->GetHandle().fd < 0) {
        asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_SocketError, errno);
        return false;
    }
    return true;
}

bool BindAndListen(AsyncNetIo *asocket, int port, int backlog)
{
    asocket->GetErrorCode() = error::ErrorCode::Error_NoError;
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if( bind(asocket->GetHandle().fd, (sockaddr*)&addr, sizeof(addr)) )
    {
        asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_BindError, errno);
        return false;
    }
    if( listen(asocket->GetHandle().fd, backlog) )
    {
        asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_ListenError, errno);
        return false;
    }
    return true;
}

coroutine::Awaiter_GHandle AsyncAccept(AsyncNetIo *asocket, NetAddr* addr)
{
    asocket->GetAction()->GetBindEvent()->ResetNetWaitEventType(event::kTcpWaitEventTypeAccept);
    asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return coroutine::Awaiter_GHandle(asocket->GetAction(), addr);  
}

coroutine::Awaiter_bool AsyncConnect(AsyncNetIo *asocket, NetAddr* addr)
{
    asocket->GetAction()->GetBindEvent()->ResetNetWaitEventType(event::kTcpWaitEventTypeConnect);
    asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return coroutine::Awaiter_bool(asocket->GetAction(), addr);
}

coroutine::Awaiter_int AsyncRecv(AsyncNetIo *asocket, NetIOVec* iov)
{
    asocket->GetAction()->GetBindEvent()->ResetNetWaitEventType(event::kTcpWaitEventTypeRecv);
    asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return coroutine::Awaiter_int(asocket->GetAction(), iov);
}

coroutine::Awaiter_int AsyncSend(AsyncNetIo *asocket, NetIOVec *iov)
{
    asocket->GetAction()->GetBindEvent()->ResetNetWaitEventType(event::kTcpWaitEventTypeSend);
    asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return coroutine::Awaiter_int(asocket->GetAction(), iov);
}

coroutine::Awaiter_bool AsyncClose(AsyncNetIo *asocket)
{
    asocket->GetAction()->GetBindEvent()->ResetNetWaitEventType(event::kTcpWaitEventTypeClose);
    asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return coroutine::Awaiter_bool(asocket->GetAction(), nullptr);
}

bool AsyncSSLSocket(AsyncSslNetIo* asocket, SSL_CTX *ctx)
{
    SSL* ssl = SSL_new(ctx);
    if(ssl == nullptr) return false;
    if(SSL_set_fd(ssl, asocket->GetHandle().fd)) {
        asocket->GetSSL() = ssl;
        return true;
    }
    SSL_free(ssl);
    return false;
}

coroutine::Awaiter_bool AsyncSSLAccept(AsyncSslNetIo *asocket)
{
    asocket->GetAction()->GetBindEvent()->ResetNetWaitEventType(event::kTcpWaitEventTypeSslAccept);
    asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return coroutine::Awaiter_bool(asocket->GetAction(), nullptr);
}

coroutine::Awaiter_bool SSLConnect(AsyncSslNetIo *asocket)
{
    asocket->GetAction()->GetBindEvent()->ResetNetWaitEventType(event::kTcpWaitEventTypeSslConnect);
    asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return coroutine::Awaiter_bool(asocket->GetAction(), nullptr);
}

coroutine::Awaiter_int AsyncSSLRecv(AsyncSslNetIo *asocket, NetIOVec *iov)
{
    asocket->GetAction()->GetBindEvent()->ResetNetWaitEventType(event::kTcpWaitEventTypeSslRecv);
    asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return coroutine::Awaiter_int(asocket->GetAction(), iov);
}

coroutine::Awaiter_int AsyncSSLSend(AsyncSslNetIo *asocket, NetIOVec *iov)
{
    asocket->GetAction()->GetBindEvent()->ResetNetWaitEventType(event::kTcpWaitEventTypeSslSend);
    asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return coroutine::Awaiter_int(asocket->GetAction(), iov);
}

coroutine::Awaiter_bool AsyncSSLClose(AsyncSslNetIo *asocket)
{
    asocket->GetAction()->GetBindEvent()->ResetNetWaitEventType(event::kTcpWaitEventTypeSslClose);
    asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return coroutine::Awaiter_bool(asocket->GetAction(), nullptr);
}

coroutine::Awaiter_GHandle AsyncFileOpen(const char *path, int flags, mode_t mode)
{
#if defined(__linux__) || defined(__APPLE__) 
    int fd = open(path, flags);
#endif
    GHandle handle = GHandle{fd};
    HandleOption option(handle);
    option.HandleNonBlock();
    return coroutine::Awaiter_GHandle(GHandle{fd});
}

coroutine::Awaiter_int AsyncFileRead(AsyncFileIo *afileio, FileIOVec *iov)
{
    afileio->GetAction()->GetBindEvent()->ResetFileIoWaitEventType(event::kFileIoWaitEventTypeRead);
    afileio->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return coroutine::Awaiter_int(afileio->GetAction(), iov);
}

coroutine::Awaiter_int AsyncFileWrite(AsyncFileIo *afileio, FileIOVec *iov)
{
    afileio->GetAction()->GetBindEvent()->ResetFileIoWaitEventType(event::kFileIoWaitEventTypeWrite);
    afileio->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return coroutine::Awaiter_int(afileio->GetAction(), iov);
}

AsyncNetIo::AsyncNetIo(event::EventEngine* engine)
{
    event::NetWaitEvent* event = new event::NetWaitEvent(this);
    this->m_action = new action::NetEventAction(engine, event);
}

HandleOption
AsyncNetIo::GetOption()
{
    return HandleOption(this->m_handle);
}

AsyncNetIo::~AsyncNetIo()
{
    delete m_action;
}


uint32_t &AsyncNetIo::GetErrorCode()
{
    return this->m_err_code;
}

AsyncTcpSocket::AsyncTcpSocket(event::EventEngine *engine)
    :AsyncNetIo(engine)
{
}

AsyncSslNetIo::AsyncSslNetIo(event::EventEngine *engine)
    : AsyncNetIo(engine)
{
}

AsyncSslNetIo::~AsyncSslNetIo()
{
    if(m_ssl) {
        SSL_shutdown(m_ssl);
        SSL_free(m_ssl);
    }
}

AsyncSslTcpSocket::AsyncSslTcpSocket(event::EventEngine *engine)
    :AsyncSslNetIo(engine)
{
}

AsyncUdpSocket::AsyncUdpSocket(event::EventEngine *engine)
    :AsyncNetIo(engine)
{
}

AsyncUdpSocket::~AsyncUdpSocket()
{
}

AsyncFileIo::AsyncFileIo(event::EventEngine* engine)
{
    event::FileIoWaitEvent* event = new event::FileIoWaitEvent(this);
    m_action = new action::FileIoEventAction(engine, event);
}

HandleOption AsyncFileIo::GetOption()
{
    return HandleOption(m_handle);
}

AsyncFileIo::~AsyncFileIo()
{
    delete m_action;
}

#ifdef __linux__ 

AsyncFileNativeAio::AsyncFileNativeAio(int maxevents, event::EventEngine *engine)
    :AsyncFileIo(engine), m_current_index(0), m_unfinished_io(0)
{
    int ret = io_setup(maxevents, &m_ioctx);
    if(ret) {
        GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_LinuxAioSetupError, errno);
        return;
    }
    GetHandle().fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC | EFD_SEMAPHORE);
    m_iocbs.resize(maxevents);
    m_iocb_ptrs.resize(maxevents); 
    for (int i = 0 ; i < maxevents; ++i) {
        m_iocb_ptrs[i] = &m_iocbs[i];   
    }                                                                                                                                                                                                                                                                                                               
}

bool AsyncFileNativeAio::PrepareRead(GHandle handle, char *buf, size_t len, long long offset)
{
    uint32_t num = m_current_index.load();
    if(num >= m_iocbs.size()) return false;
    if(!m_current_index.compare_exchange_strong(num, num + 1)) return false;
    if(!m_unfinished_io.compare_exchange_strong(num, num + 1)) return false;
    io_prep_pread(m_iocb_ptrs[num], handle.fd, buf, len, offset);
    io_set_eventfd(m_iocb_ptrs[num], GetHandle().fd);
    return true;
}

bool AsyncFileNativeAio::PrepareWrite(GHandle handle, char *buf, size_t len, long long offset)
{
    uint32_t num = m_current_index.load();
    if(num >= m_iocbs.size()) return false;
    if(!m_current_index.compare_exchange_strong(num, num + 1)) return false;
    if(!m_unfinished_io.compare_exchange_strong(num, num + 1)) return false;
    io_prep_pwrite(m_iocb_ptrs[num], handle.fd, buf, len, offset);
    io_set_eventfd(m_iocb_ptrs[num], GetHandle().fd);
    return true;
}

bool AsyncFileNativeAio::PrepareReadV(GHandle handle, iovec* iov, int count, long long offset)
{
    uint32_t num = m_current_index.load();
    if(num >= m_iocbs.size()) return false;
    if(!m_current_index.compare_exchange_strong(num, num + 1)) return false;
    if(!m_unfinished_io.compare_exchange_strong(num, num + 1)) return false;
    io_prep_preadv(m_iocb_ptrs[num], handle.fd, iov, count, offset);
    io_set_eventfd(m_iocb_ptrs[num], GetHandle().fd);
    return true;
}

bool AsyncFileNativeAio::PrepareWriteV(GHandle handle, iovec* iov, int count, long long offset)
{
    uint32_t num = m_current_index.load();
    if(num >= m_iocbs.size()) return false;
    if(!m_current_index.compare_exchange_strong(num, num + 1)) return false;
    if(!m_unfinished_io.compare_exchange_strong(num, num + 1)) return false;
    io_prep_pwritev(m_iocb_ptrs[num], handle.fd, iov, count, offset);
    io_set_eventfd(m_iocb_ptrs[num], GetHandle().fd);
    return true;
}

coroutine::Awaiter_int AsyncFileNativeAio::Commit()
{
    int ret = io_submit(m_ioctx, m_unfinished_io, m_iocb_ptrs.data());
    if( ret < 0 ) {
        GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_LinuxAioSubmitError, -ret);
        return coroutine::Awaiter_int(-1);
    }
    m_current_index = 0;
    GetAction()->GetBindEvent()->ResetFileIoWaitEventType(event::kFileIoWaitEventTypeLinuxAio);
    return coroutine::Awaiter_int(GetAction(), nullptr);
}

bool AsyncFileNativeAio::IoFinished(uint64_t finished_io)
{
    uint32_t num = m_unfinished_io.load();
    if(!m_unfinished_io.compare_exchange_strong(num, num - finished_io)) return false;
    return m_unfinished_io == 0;
}

AsyncFileNativeAio::~AsyncFileNativeAio()
{
    io_destroy(m_ioctx);
}

#endif

AsyncFileDiscreptor::AsyncFileDiscreptor(event::EventEngine *engine)
    :AsyncFileIo(engine)
{
}

}