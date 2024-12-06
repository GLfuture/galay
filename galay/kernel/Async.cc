#include "Async.h"
#include "Event.h"
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

namespace galay::async
{

HandleOption::HandleOption(const GHandle handle)
    : m_handle(handle), m_error_code(0)
{
    
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
    constexpr int option = 1;
    if (const int ret = setsockopt(this->m_handle.fd, SOL_SOCKET, SO_REUSEPORT, &option, sizeof(option)); ret < 0) {
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

AsyncNetIo::AsyncNetIo(details::EventEngine* engine)
    : m_handle({}), m_err_code(0), m_action(new details::IOEventAction(engine, new details::NetWaitEvent(this)))
{
}

HandleOption
AsyncNetIo::GetOption() const
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

AsyncTcpSocket::AsyncTcpSocket(details::EventEngine *engine)
    :AsyncNetIo(engine)
{
}

AsyncSslNetIo::AsyncSslNetIo(details::EventEngine *engine)
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

AsyncSslTcpSocket::AsyncSslTcpSocket(details::EventEngine *engine)
    :AsyncSslNetIo(engine)
{
}

AsyncUdpSocket::AsyncUdpSocket(details::EventEngine *engine)
    :AsyncNetIo(engine), m_handle({}), m_error_code(0)
{
}

AsyncUdpSocket::~AsyncUdpSocket()
= default;

AsyncFileIo::AsyncFileIo(details::EventEngine* engine)
    :m_handle({}), m_error_code(0), m_action(new details::IOEventAction(engine, new details::FileIoWaitEvent(this)))
{

}

HandleOption AsyncFileIo::GetOption() const
{
    return HandleOption(m_handle);
}

AsyncFileIo::~AsyncFileIo()
{
    delete m_action;
}

#ifdef __linux__ 

AsyncFileNativeAio::AsyncFileNativeAio(int maxevents, details::EventEngine *engine)
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

bool AsyncFileNativeAio::PrepareRead(GHandle handle, char *buf, size_t len, long long offset, AioCallback* callback)
{
    uint32_t num = m_current_index.load();
    if(num >= m_iocbs.size()) return false;
    if(!m_current_index.compare_exchange_strong(num, num + 1)) return false;
    if(!m_unfinished_io.compare_exchange_strong(num, num + 1)) return false;
    io_prep_pread(m_iocb_ptrs[num], handle.fd, buf, len, offset);
    io_set_eventfd(m_iocb_ptrs[num], GetHandle().fd);
    m_iocb_ptrs[num]->data = callback;
    return true;
}

bool AsyncFileNativeAio::PrepareWrite(GHandle handle, char *buf, size_t len, long long offset, AioCallback* callback)
{
    uint32_t num = m_current_index.load();
    if(num >= m_iocbs.size()) return false;
    if(!m_current_index.compare_exchange_strong(num, num + 1)) return false;
    if(!m_unfinished_io.compare_exchange_strong(num, num + 1)) return false;
    io_prep_pwrite(m_iocb_ptrs[num], handle.fd, buf, len, offset);
    io_set_eventfd(m_iocb_ptrs[num], GetHandle().fd);
    m_iocb_ptrs[num]->data = this;
    return true;
}

bool AsyncFileNativeAio::PrepareReadV(GHandle handle, iovec* iov, int count, long long offset, AioCallback* callback)
{
    uint32_t num = m_current_index.load();
    if(num >= m_iocbs.size()) return false;
    if(!m_current_index.compare_exchange_strong(num, num + 1)) return false;
    if(!m_unfinished_io.compare_exchange_strong(num, num + 1)) return false;
    io_prep_preadv(m_iocb_ptrs[num], handle.fd, iov, count, offset);
    io_set_eventfd(m_iocb_ptrs[num], GetHandle().fd);
    m_iocb_ptrs[num]->data = callback;
    return true;
}

bool AsyncFileNativeAio::PrepareWriteV(GHandle handle, iovec* iov, int count, long long offset, AioCallback* callback)
{
    uint32_t num = m_current_index.load();
    if(num >= m_iocbs.size()) return false;
    if(!m_current_index.compare_exchange_strong(num, num + 1)) return false;
    if(!m_unfinished_io.compare_exchange_strong(num, num + 1)) return false;
    io_prep_pwritev(m_iocb_ptrs[num], handle.fd, iov, count, offset);
    io_set_eventfd(m_iocb_ptrs[num], GetHandle().fd);
    m_iocb_ptrs[num]->data = callback;
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
    dynamic_cast<details::FileIoWaitEvent*>(GetAction()->GetBindEvent())->ResetFileIoWaitEventType(details::kFileIoWaitEventTypeLinuxAio);
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

AsyncFileDescriptor::AsyncFileDescriptor(details::EventEngine *engine)
    :AsyncFileIo(engine)
{
}

}