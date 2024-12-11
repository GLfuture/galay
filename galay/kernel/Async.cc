#include "Async.h"
#include "Event.h"
#include "WaitAction.h"
#include "ExternApi.h"
#include "Internal.h"
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

AsyncNetIo::AsyncNetIo(details::EventEngine* engine)
    : m_handle({}), m_err_code(0), m_action(new details::IOEventAction(engine, new details::NetWaitEvent(this)))
{
}

AsyncNetIo::AsyncNetIo(GHandle handle, details::EventEngine *engine)
    :m_handle(handle), m_err_code(0)
{
    ActionInit(engine);
}

HandleOption
AsyncNetIo::GetOption() const
{
    return HandleOption(m_handle);
}

AsyncNetIo::~AsyncNetIo()
{
    delete m_action;
}

void AsyncNetIo::ActionInit(details::EventEngine *engine)
{
    m_action = new details::IOEventAction(engine, new details::NetWaitEvent(this));
}

uint32_t &AsyncNetIo::GetErrorCode()
{
    return m_err_code;
}

AsyncSslNetIo::AsyncSslNetIo(details::EventEngine *engine)
    : AsyncNetIo(engine), m_ssl(nullptr)
{
    ActionInit(engine);
}

AsyncSslNetIo::AsyncSslNetIo(GHandle handle, details::EventEngine *engine)
    :AsyncNetIo(handle, engine), m_ssl(nullptr)
{
}

AsyncSslNetIo::AsyncSslNetIo(SSL *ssl, details::EventEngine *engine)
    :AsyncNetIo({SSL_get_fd(ssl)}, engine), m_ssl(ssl)
{
}

AsyncSslNetIo::~AsyncSslNetIo()
{
    if(m_ssl) {
        SSL_shutdown(m_ssl);
        SSL_free(m_ssl);
    }
}

void AsyncSslNetIo::ActionInit(details::EventEngine *engine)
{
    m_action = new details::IOEventAction(engine, new details::NetSslWaitEvent(this));
}

AsyncFileIo::AsyncFileIo(details::EventEngine* engine)
    :m_handle({}), m_error_code(0), m_action(new details::IOEventAction(engine, new details::FileIoWaitEvent(this)))
{

}

AsyncFileIo::AsyncFileIo(GHandle handle, details::EventEngine *engine)
    :m_handle(handle), m_error_code(0), m_action(new details::IOEventAction(engine, new details::FileIoWaitEvent(this)))
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

AsyncFileNativeAio::AsyncFileNativeAio(details::EventEngine *engine, int maxevents)
    :AsyncFileIo(engine), m_current_index(0), m_unfinished_io(0)
{
    int ret = io_setup(maxevents, &m_ioctx);
    if(ret) {
        GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_LinuxAioSetupError, errno);
        return;
    }
    m_iocbs.resize(maxevents);
    m_iocb_ptrs.resize(maxevents); 
    for (int i = 0 ; i < maxevents; ++i) {
        m_iocb_ptrs[i] = &m_iocbs[i];   
    }
}
AsyncFileNativeAio::AsyncFileNativeAio(GHandle handle, details::EventEngine *engine, int maxevents)
    :AsyncFileIo(handle, engine), m_current_index(0), m_unfinished_io(0)
{
    int ret = io_setup(maxevents, &m_ioctx);
    if(ret) {
        GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_LinuxAioSetupError, errno);
        return;
    }
    m_iocbs.resize(maxevents);
    m_iocb_ptrs.resize(maxevents); 
    for (int i = 0 ; i < maxevents; ++i) {
        m_iocb_ptrs[i] = &m_iocbs[i];   
    }
}

bool AsyncFileNativeAio::InitialEventHandle()
{
    m_event_handle.fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC | EFD_SEMAPHORE);
    if(m_event_handle.fd < 0) return false;
    return true;
}

bool AsyncFileNativeAio::InitialEventHandle(GHandle handle)
{
    m_event_handle.fd = handle.fd;
    return true;
}

bool AsyncFileNativeAio::CloseEventHandle()
{
    int ret = close(m_event_handle.fd);
    if(ret < 0) return false;
    m_event_handle.fd = 0;
    return true;
}

bool AsyncFileNativeAio::PrepareRead(char *buf, size_t len, long long offset, AioCallback* callback)
{
    uint32_t num = m_current_index.load();
    if(num >= m_iocbs.size()) return false;
    if(!m_current_index.compare_exchange_strong(num, num + 1)) return false;
    if(!m_unfinished_io.compare_exchange_strong(num, num + 1)) return false;
    io_prep_pread(m_iocb_ptrs[num], m_handle.fd, buf, len, offset);
    io_set_eventfd(m_iocb_ptrs[num], m_event_handle.fd);
    m_iocb_ptrs[num]->data = callback;
    return true;
}

bool AsyncFileNativeAio::PrepareWrite(char *buf, size_t len, long long offset, AioCallback* callback)
{
    uint32_t num = m_current_index.load();
    if(num >= m_iocbs.size()) return false;
    if(!m_current_index.compare_exchange_strong(num, num + 1)) return false;
    if(!m_unfinished_io.compare_exchange_strong(num, num + 1)) return false;
    io_prep_pwrite(m_iocb_ptrs[num], m_handle.fd, buf, len, offset);
    io_set_eventfd(m_iocb_ptrs[num], m_event_handle.fd);
    m_iocb_ptrs[num]->data = callback;
    return true;
}

bool AsyncFileNativeAio::PrepareReadV(iovec* iov, int count, long long offset, AioCallback* callback)
{
    uint32_t num = m_current_index.load();
    if(num >= m_iocbs.size()) return false;
    if(!m_current_index.compare_exchange_strong(num, num + 1)) return false;
    if(!m_unfinished_io.compare_exchange_strong(num, num + 1)) return false;
    io_prep_preadv(m_iocb_ptrs[num], m_handle.fd, iov, count, offset);
    io_set_eventfd(m_iocb_ptrs[num], m_event_handle.fd);
    m_iocb_ptrs[num]->data = callback;
    return true;
}

bool AsyncFileNativeAio::PrepareWriteV(iovec* iov, int count, long long offset, AioCallback* callback)
{
    uint32_t num = m_current_index.load();
    if(num >= m_iocbs.size()) return false;
    if(!m_current_index.compare_exchange_strong(num, num + 1)) return false;
    if(!m_unfinished_io.compare_exchange_strong(num, num + 1)) return false;
    io_prep_pwritev(m_iocb_ptrs[num], m_handle.fd, iov, count, offset);
    io_set_eventfd(m_iocb_ptrs[num], m_event_handle.fd);
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
    return {GetAction(), nullptr};
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
    close(m_event_handle.fd);
}

#endif


AsyncTcpSocket::AsyncTcpSocket(details::EventEngine *engine)
    :m_io(std::make_shared<AsyncNetIo>(engine))
{
}

AsyncTcpSocket::AsyncTcpSocket(GHandle handle, details::EventEngine *engine)
    :m_io(std::make_shared<AsyncNetIo>(handle, engine))
{
}

bool AsyncTcpSocket::Socket() const
{
    return details::AsyncTcpSocket(m_io);
}

bool AsyncTcpSocket::Socket(GHandle handle)
{
    m_io->GetHandle() = handle;
    HandleOption option(handle);
    return option.HandleNonBlock();
}

bool AsyncTcpSocket::Bind(const std::string& addr, int port)
{
    return details::Bind(m_io, addr, port);
}

bool AsyncTcpSocket::Listen(int backlog)
{
    return details::Listen(m_io, backlog);
}

coroutine::Awaiter_bool AsyncTcpSocket::Connect(NetAddr *addr)
{
    return details::AsyncConnect(m_io, addr);
}

coroutine::Awaiter_GHandle AsyncTcpSocket::Accept(NetAddr *addr)
{
    return details::AsyncAccept(m_io, addr);
}

coroutine::Awaiter_int AsyncTcpSocket::Recv(TcpIOVec *iov, size_t length)
{
    return details::AsyncRecv(m_io, iov, length);
}

coroutine::Awaiter_int AsyncTcpSocket::Send(TcpIOVec *iov, size_t length)
{
    return details::AsyncSend(m_io, iov, length);
}

coroutine::Awaiter_bool AsyncTcpSocket::Close()
{
    return details::AsyncNetClose(m_io);
}


AsyncTcpSslSocket::AsyncTcpSslSocket(details::EventEngine *engine)
    :m_io(std::make_shared<AsyncSslNetIo>(engine))
{
    if(!SslCtx) {
        SslCtx = GetGlobalSSLCtx();
    }
}

AsyncTcpSslSocket::AsyncTcpSslSocket(GHandle handle, details::EventEngine *engine)
    :m_io(std::make_shared<AsyncSslNetIo>(handle, engine))
{
    if(!SslCtx) {
        SslCtx = GetGlobalSSLCtx();
    }
}

SSL_CTX* AsyncTcpSslSocket::SslCtx = nullptr;

AsyncTcpSslSocket::AsyncTcpSslSocket(SSL *ssl, details::EventEngine *engine)
    :m_io(std::make_shared<AsyncSslNetIo>(ssl, engine))
{
    if(!SslCtx) {
        SslCtx = GetGlobalSSLCtx();
    }
}

bool AsyncTcpSslSocket::Socket() const
{
    if(!details::AsyncTcpSocket(m_io)){
        return false;
    }
    return details::AsyncSSLSocket(m_io, SslCtx);
}

bool AsyncTcpSslSocket::Socket(GHandle handle)
{
    m_io->GetHandle() = handle;
    HandleOption option(handle);
    if(!option.HandleNonBlock()) {
        return false;
    }
    return details::AsyncSSLSocket(m_io, SslCtx);;
}

bool AsyncTcpSslSocket::Bind(const std::string& addr, int port)
{
    return details::Bind(m_io, addr, port);
}

bool AsyncTcpSslSocket::Listen(int backlog)
{
    return details::Listen(m_io, backlog);
}

coroutine::Awaiter_bool AsyncTcpSslSocket::Connect(NetAddr *addr)
{
    return details::AsyncConnect(m_io, addr);
}

coroutine::Awaiter_bool AsyncTcpSslSocket::AsyncSSLConnect()
{
    return details::AsyncSSLConnect(m_io);
}

coroutine::Awaiter_GHandle AsyncTcpSslSocket::Accept(NetAddr *addr)
{
    return details::AsyncAccept(m_io, addr);
}

coroutine::Awaiter_bool AsyncTcpSslSocket::SSLAccept()
{
    return details::AsyncSSLAccept(m_io);
}

coroutine::Awaiter_int AsyncTcpSslSocket::Recv(TcpIOVec *iov, size_t length)
{
    return details::AsyncSSLRecv(m_io, iov, length);
}

coroutine::Awaiter_int AsyncTcpSslSocket::Send(TcpIOVec *iov, size_t length)
{
    return details::AsyncSSLSend(m_io, iov, length);
}

coroutine::Awaiter_bool AsyncTcpSslSocket::Close()
{
    return details::AsyncSSLClose(m_io);
}

AsyncUdpSocket::AsyncUdpSocket(details::EventEngine *engine)
    :m_io(std::make_shared<AsyncNetIo>(engine))
{
}

AsyncUdpSocket::AsyncUdpSocket(GHandle handle, details::EventEngine *engine)
    :m_io(std::make_shared<AsyncNetIo>(handle, engine))
{
}

bool AsyncUdpSocket::Socket() const
{
    return details::AsyncUdpSocket(m_io);
}

bool AsyncUdpSocket::Bind(const std::string& addr, int port)
{
    return details::Bind(m_io, addr, port);
}

coroutine::Awaiter_int AsyncUdpSocket::RecvFrom(UdpIOVec *iov, size_t length)
{
    return details::AsyncRecvFrom(m_io, iov, length);
}

coroutine::Awaiter_int AsyncUdpSocket::SendTo(UdpIOVec *iov, size_t length)
{
    return details::AsyncSendTo(m_io, iov, length);
}

coroutine::Awaiter_bool AsyncUdpSocket::Close()
{
    return details::AsyncNetClose(m_io);
}


AsyncFileDescriptor::AsyncFileDescriptor(details::EventEngine *engine)
    :m_io(std::make_shared<AsyncFileIo>(engine))
{
}

AsyncFileDescriptor::AsyncFileDescriptor(GHandle handle, details::EventEngine *engine)
    :m_io(std::make_shared<AsyncFileIo>(handle, engine))
{
}

bool AsyncFileDescriptor::Open(const char *path, int flags, mode_t mode)
{
    return details::AsyncFileOpen(m_io, path, flags, mode);
}


coroutine::Awaiter_int AsyncFileDescriptor::Read(FileIOVec* iov, size_t length)
{
    return details::AsyncFileRead(m_io, iov, length);
}


coroutine::Awaiter_int AsyncFileDescriptor::Write(FileIOVec* iov, size_t length)
{
    return details::AsyncFileWrite(m_io, iov, length);
}


coroutine::Awaiter_bool AsyncFileDescriptor::Close()
{
    return details::AsyncFileClose(m_io);
}


#ifdef __linux__

AsyncFileNativeAioDescriptor::AsyncFileNativeAioDescriptor(details::EventEngine *engine, int maxevents)
    :m_io(std::make_shared<AsyncFileNativeAio>(engine, maxevents))
{
    m_io->InitialEventHandle();
}

AsyncFileNativeAioDescriptor::AsyncFileNativeAioDescriptor(GHandle handle, details::EventEngine *engine, int maxevents)
    :m_io(std::make_shared<AsyncFileNativeAio>(handle, engine, maxevents))
{
    m_io->InitialEventHandle();
}

bool AsyncFileNativeAioDescriptor::Open(const char *path, int flags, mode_t mode)
{
    return details::AsyncFileOpen(m_io, path, flags, mode);
}

bool AsyncFileNativeAioDescriptor::PrepareRead(char *buf, size_t len, long long offset, AioCallback *callback)
{
    return m_io->PrepareRead(buf, len, offset, callback);
}


bool AsyncFileNativeAioDescriptor::PrepareWrite(char *buf, size_t len, long long offset, AioCallback *callback)
{
    return m_io->PrepareWrite(buf, len, offset, callback);
}

bool AsyncFileNativeAioDescriptor::PrepareReadV(iovec *iov, int count, long long offset, AioCallback *callback)
{
    return m_io->PrepareReadV(iov, count, offset, callback);
}


bool AsyncFileNativeAioDescriptor::PrepareWriteV(iovec *iov, int count, long long offset, AioCallback *callback)
{
    return m_io->PrepareWriteV(iov, count, offset, callback);
}

coroutine::Awaiter_int AsyncFileNativeAioDescriptor::Commit()
{
    return m_io->Commit();
}

coroutine::Awaiter_bool AsyncFileNativeAioDescriptor::Close()
{
    return details::AsyncFileClose(m_io);
}


AsyncFileNativeAioDescriptor::~AsyncFileNativeAioDescriptor()
{
    m_io->CloseEventHandle();
}

#endif
}