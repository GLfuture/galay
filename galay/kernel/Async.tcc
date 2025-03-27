#ifndef GALAY_ASYNC_TCC
#define GALAY_ASYNC_TCC

#include "Async.hpp"
#if defined(__linux__)
    #include <sys/eventfd.h>
#endif

namespace galay::details
{
inline bool AsyncTcpSocket(AsyncNetEventContext* async_context, bool noblock)
{
    async_context->m_error_code = error::ErrorCode::Error_NoError;
    async_context->m_handle.fd = socket(AF_INET, SOCK_STREAM, 0);
    if (async_context->m_handle.fd < 0) {
        async_context->m_error_code = error::MakeErrorCode(error::ErrorCode::Error_SocketError, errno);
        return false;
    }
    if(noblock) {
        HandleOption option(async_context->m_handle);
        option.HandleNonBlock();
    }
    return true;
}


inline bool AsyncUdpSocket(AsyncNetEventContext* async_context, bool noblock)
{
    async_context->m_error_code = error::ErrorCode::Error_NoError;
    async_context->m_handle.fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (async_context->m_handle.fd < 0) {
        async_context->m_error_code = error::MakeErrorCode(error::ErrorCode::Error_SocketError, errno);
        return false;
    }
    if(noblock) {
        HandleOption option(async_context->m_handle);
        option.HandleNonBlock();
    }
    return true;
}


inline bool Bind(AsyncNetEventContext* async_context, const std::string& addr, int port)
{
    async_context->m_error_code = error::ErrorCode::Error_NoError;
    sockaddr_in taddr{};
    taddr.sin_family = AF_INET;
    taddr.sin_port = htons(port);
    if(addr.empty()) taddr.sin_addr.s_addr = INADDR_ANY;
    else taddr.sin_addr.s_addr = inet_addr(addr.c_str());
    if( bind(async_context->m_handle.fd, reinterpret_cast<sockaddr*>(&taddr), sizeof(taddr)) )
    {
        async_context->m_error_code = error::MakeErrorCode(error::ErrorCode::Error_BindError, errno);
        return false;
    }
    return true;
}


inline bool Listen(AsyncNetEventContext* async_context, int backlog)
{
    async_context->m_error_code = error::ErrorCode::Error_NoError;
    if( listen(async_context->m_handle.fd, backlog) )
    {
        async_context->m_error_code = error::MakeErrorCode(error::ErrorCode::Error_ListenError, errno);
        return false;
    }
    return true;
}

template<typename CoRtn>
inline AsyncResult<GHandle, CoRtn> AsyncAccept(AsyncNetEventContext* async_context, THost* addr, int64_t timeout)
{
    static_cast<NetWaitEvent*>(async_context->m_action->GetBindEvent())->ResetNetWaitEventType(kTcpWaitEventTypeAccept);
    async_context->m_error_code = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    async_context->m_resumer->SetResumeInterfaceType(kResumeInterfaceType_Accept);
    async_context->m_timeout = timeout;
    return {async_context->m_action.get(), addr};
}

template<typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncConnect(AsyncNetEventContext* async_context, THost* addr, int64_t timeout)
{
    static_cast<NetWaitEvent*>(async_context->m_action->GetBindEvent())->ResetNetWaitEventType(kTcpWaitEventTypeConnect);
    async_context->m_error_code = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    async_context->m_resumer->SetResumeInterfaceType(kResumeInterfaceType_Connect);
    async_context->m_timeout = timeout;
    return {async_context->m_action.get(), addr};
}

template<typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncRecv(AsyncNetEventContext* async_context, TcpIOVec* iov, size_t length, int64_t timeout)
{
    static_cast<NetWaitEvent*>(async_context->m_action->GetBindEvent())->ResetNetWaitEventType(kTcpWaitEventTypeRecv);
    async_context->m_error_code = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    async_context->m_resumer->SetResumeInterfaceType(kResumeInterfaceType_Recv);
    async_context->m_timeout = timeout;
    iov->m_length = length;
    if(iov->m_buffer == nullptr) {
        iov->m_buffer = new char[length];
    }
    return {async_context->m_action.get(), iov};
}

template<typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncSend(AsyncNetEventContext* async_context, TcpIOVec *iov, size_t length, int64_t timeout)
{
    static_cast<NetWaitEvent*>(async_context->m_action->GetBindEvent())->ResetNetWaitEventType(kTcpWaitEventTypeSend);
    async_context->m_error_code = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    async_context->m_resumer->SetResumeInterfaceType(kResumeInterfaceType_Send);
    async_context->m_timeout = timeout;
    iov->m_length = length;
    return {async_context->m_action.get(), iov};
}

template<typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncRecvFrom(AsyncNetEventContext* async_context, UdpIOVec *iov, size_t length, int64_t timeout)
{
    static_cast<NetWaitEvent*>(async_context->m_action->GetBindEvent())->ResetNetWaitEventType(kUdpWaitEventTypeRecvFrom);
    async_context->m_error_code = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    async_context->m_resumer->SetResumeInterfaceType(kResumeInterfaceType_RecvFrom);
    async_context->m_timeout = timeout;
    iov->m_length = length;
    return {async_context->m_action.get(), iov};
}

template<typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncSendTo(AsyncNetEventContext* async_context, UdpIOVec *iov, size_t length, int64_t timeout)
{
    static_cast<NetWaitEvent*>(async_context->m_action->GetBindEvent())->ResetNetWaitEventType(kUdpWaitEventTypeSendTo);
    async_context->m_error_code = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    async_context->m_resumer->SetResumeInterfaceType(kResumeInterfaceType_SendTo);
    async_context->m_timeout = timeout;
    iov->m_length = length;
    return {async_context->m_action.get(), iov};
}

template<typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncNetClose(AsyncNetEventContext* async_context)
{
    static_cast<NetWaitEvent*>(async_context->m_action->GetBindEvent())->ResetNetWaitEventType(kWaitEventTypeClose);
    async_context->m_error_code = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return {async_context->m_action.get(), nullptr};
}

inline bool AsyncSSLSocket(AsyncSslNetEventContext* async_context, SSL_CTX *ctx)
{
    SSL* ssl = SSL_new(ctx);
    if(ssl == nullptr) return false;
    if(SSL_set_fd(ssl, async_context->m_handle.fd)) {
        async_context->m_ssl = ssl;
        return true;
    }
    SSL_free(ssl);
    return false;
}

template<typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncSSLAccept(AsyncSslNetEventContext* async_context, int64_t timeout)
{
    static_cast<NetWaitEvent*>(async_context->m_action->GetBindEvent())->ResetNetWaitEventType(kTcpWaitEventTypeSslAccept);
    async_context->m_error_code = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    async_context->m_resumer->SetResumeInterfaceType(kResumeInterfaceType_SSLAccept);
    async_context->m_timeout = timeout;
    return {async_context->m_action.get(), nullptr};
}

template<typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncSSLConnect(AsyncSslNetEventContext* async_context, int64_t timeout)
{
    static_cast<NetWaitEvent*>(async_context->m_action->GetBindEvent())->ResetNetWaitEventType(kTcpWaitEventTypeSslConnect);
    async_context->m_error_code = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    async_context->m_resumer->SetResumeInterfaceType(kResumeInterfaceType_SSLConnect);
    async_context->m_timeout = timeout;
    return {async_context->m_action.get(), nullptr};
}

template<typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncSSLRecv(AsyncSslNetEventContext* async_context, TcpIOVec *iov, size_t length, int64_t timeout)
{
    static_cast<NetWaitEvent*>(async_context->m_action->GetBindEvent())->ResetNetWaitEventType(kTcpWaitEventTypeSslRecv);
    async_context->m_error_code = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    async_context->m_resumer->SetResumeInterfaceType(kResumeInterfaceType_SSLRead);
    async_context->m_timeout = timeout;
    iov->m_length = length;
    return {async_context->m_action.get(), iov};
}

template<typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncSSLSend(AsyncSslNetEventContext* async_context, TcpIOVec *iov, size_t length, int64_t timeout)
{
    static_cast<NetWaitEvent*>(async_context->m_action->GetBindEvent())->ResetNetWaitEventType(kTcpWaitEventTypeSslSend);
    async_context->m_error_code = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    async_context->m_resumer->SetResumeInterfaceType(kResumeInterfaceType_SSLWrite);
    async_context->m_timeout = timeout;
    iov->m_length = length;
    return {async_context->m_action.get(), iov};
}

template<typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncSSLClose(AsyncSslNetEventContext* async_context)
{
    static_cast<NetWaitEvent*>(async_context->m_action->GetBindEvent())->ResetNetWaitEventType(kWaitEventTypeSslClose);
    async_context->m_error_code = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return {async_context->m_action.get(), nullptr};
}


inline bool AsyncFileOpen(AsyncFileEventContext* async_context, const char *path, const int flags, mode_t mode)
{
#if defined(__linux__) || defined(__APPLE__) 
    const int fd = open(path, flags, mode);
    if( fd < 0 ) {
        return false;
    }
#endif
    async_context->m_handle = GHandle{fd};
    HandleOption option(GHandle{fd});
    option.HandleNonBlock();
    return true;
}

template<typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncFileRead(AsyncFileEventContext* async_context, FileIOVec *iov,  size_t length, int64_t timeout)
{
    static_cast<FileIoWaitEvent*>(async_context->m_action->GetBindEvent())->ResetFileIoWaitEventType(kFileIoWaitEventTypeRead);
    async_context->m_error_code = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    async_context->m_resumer->SetResumeInterfaceType(kResumeInterfaceType_FileRead);
    async_context->m_timeout = timeout;
    iov->m_length = length;
    return {async_context->m_action.get(), iov};
}

template<typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncFileWrite(AsyncFileEventContext* async_context, FileIOVec *iov, size_t length, int64_t timeout)
{
    static_cast<FileIoWaitEvent*>(async_context->m_action->GetBindEvent())->ResetFileIoWaitEventType(kFileIoWaitEventTypeWrite);
    async_context->m_error_code = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    async_context->m_resumer->SetResumeInterfaceType(kResumeInterfaceType_FileWrite);
    async_context->m_timeout = timeout;
    iov->m_length = length;
    return {async_context->m_action.get(), iov};
}

template<typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncFileClose(AsyncFileEventContext* async_context)
{
    static_cast<FileIoWaitEvent*>(async_context->m_action->GetBindEvent())->ResetFileIoWaitEventType(kFileIoWaitEventTypeClose);
    async_context->m_error_code = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return {async_context->m_action.get(), nullptr};
}

}


namespace galay
{


inline AsyncNetEventContext::uptr AsyncNetEventContext::Create()
{
    auto context = std::make_unique<AsyncNetEventContext>();
    context->m_action = std::make_unique<details::IOEventAction>(std::make_unique<details::NetWaitEvent>(context.get()));
    context->m_resumer = std::make_shared<details::Resumer>(context->m_timeout);
    return context;
}

inline AsyncNetEventContext::uptr AsyncNetEventContext::Create(GHandle handle)
{
    auto context = std::make_unique<AsyncNetEventContext>();
    context->m_action = std::make_unique<details::IOEventAction>(std::make_unique<details::NetWaitEvent>(context.get()));
    context->m_resumer = std::make_shared<details::Resumer>(context->m_timeout);
    context->m_handle = handle;
    return context;
}

inline void AsyncNetEventContext::Resume()
{
    m_timeout = -1;
    m_resumer->Resume();
}


inline AsyncSslNetEventContext::uptr AsyncSslNetEventContext::Create()
{
    auto context = std::make_unique<AsyncSslNetEventContext>();
    context->m_action = std::make_unique<details::IOEventAction>(std::make_unique<details::NetSslWaitEvent>(context.get()));
    context->m_resumer = std::make_shared<details::Resumer>(context->m_timeout);
    return context;
}

inline AsyncSslNetEventContext::uptr AsyncSslNetEventContext::Create(GHandle handle)
{
    auto context = std::make_unique<AsyncSslNetEventContext>();
    context->m_action = std::make_unique<details::IOEventAction>(std::make_unique<details::NetSslWaitEvent>(context.get()));
    context->m_handle = handle;
    context->m_resumer = std::make_shared<details::Resumer>(context->m_timeout);
    return context;
}

inline AsyncSslNetEventContext::uptr AsyncSslNetEventContext::Create(SSL *ssl)
{
    auto context = std::make_unique<AsyncSslNetEventContext>();
    context->m_action = std::make_unique<details::IOEventAction>(std::make_unique<details::NetSslWaitEvent>(context.get()));
    context->m_resumer = std::make_shared<details::Resumer>(context->m_timeout);
    context->m_handle = {SSL_get_fd(ssl)};
    context->m_ssl = ssl;
    return context;
}

inline AsyncSslNetEventContext::~AsyncSslNetEventContext()
{
    if(m_ssl) {
        SSL_shutdown(m_ssl);
        SSL_free(m_ssl);
    }
}

inline AsyncFileEventContext::uptr galay::AsyncFileEventContext::Create()
{
    auto context = std::make_unique<AsyncFileEventContext>();
    context->m_action = std::make_unique<details::IOEventAction>(std::make_unique<details::FileIoWaitEvent>(context.get()));
    context->m_resumer = std::make_shared<details::Resumer>(context->m_timeout);
    return context;
}

inline AsyncFileEventContext::uptr AsyncFileEventContext::Create(GHandle handle)
{
    auto context = std::make_unique<AsyncFileEventContext>();
    context->m_action = std::make_unique<details::IOEventAction>(std::make_unique<details::FileIoWaitEvent>(context.get()));
    context->m_resumer = std::make_shared<details::Resumer>(context->m_timeout);
    context->m_handle = handle;
    return context;
}

inline void AsyncFileEventContext::Resume()
{
    m_timeout = -1;
    m_resumer->Resume();
}

#ifdef __linux__

inline AsyncLinuxFileEventContext::uptr AsyncLinuxFileEventContext::Create(int maxevents)
{
    auto context = std::make_unique<AsyncLinuxFileEventContext>(maxevents);
    context->m_action = std::make_unique<details::IOEventAction>(std::make_unique<details::FileIoWaitEvent>(context.get()));
    context->m_resumer = std::make_shared<details::Resumer>(context->m_timeout);
    return context;
}

inline AsyncLinuxFileEventContext::uptr AsyncLinuxFileEventContext::Create(GHandle handle, int maxevents)
{
    auto context = std::make_unique<AsyncLinuxFileEventContext>(maxevents);
    context->m_action = std::make_unique<details::IOEventAction>(std::make_unique<details::FileIoWaitEvent>(context.get()));
    context->m_resumer = std::make_shared<details::Resumer>(context->m_timeout);
    context->m_handle = handle;
    return context;
}

inline AsyncLinuxFileEventContext::AsyncLinuxFileEventContext(int maxevents)
    : AsyncFileEventContext(), m_current_index(0), m_unfinished_io(0)
{
    int ret = io_setup(maxevents, &m_ioctx);
    if(ret) {
        this->m_error_code = error::MakeErrorCode(error::ErrorCode::Error_LinuxAioSetupError, errno);
        return;
    }
    m_iocbs.resize(maxevents);
    m_iocb_ptrs.resize(maxevents); 
    for (int i = 0 ; i < maxevents; ++i) {
        m_iocb_ptrs[i] = &m_iocbs[i];   
    }
}


inline bool AsyncLinuxFileEventContext::IoFinished(uint64_t finished_io)
{
    uint32_t num = this->m_unfinished_io.load();
    if(!this->m_unfinished_io.compare_exchange_strong(num, num - finished_io)) return false;
    return this->m_unfinished_io == 0;
}


inline AsyncLinuxFileEventContext::~AsyncLinuxFileEventContext()
{
    io_destroy(this->m_ioctx);
    close(this->m_event_handle.fd);
}

#endif



inline AsyncTcpSocket::AsyncTcpSocket()
    :m_async_context(AsyncNetEventContext::Create())
{
}


inline AsyncTcpSocket::AsyncTcpSocket(GHandle handle)
    :m_async_context(AsyncNetEventContext::Create(handle))
{
}


inline bool AsyncTcpSocket::Socket(bool noblock) const
{
    return details::AsyncTcpSocket(m_async_context.get(), noblock);
}


inline bool AsyncTcpSocket::Socket(GHandle handle)
{
    m_async_context->m_handle = handle;
    HandleOption option(handle);
    return option.HandleNonBlock();
}


inline bool AsyncTcpSocket::Bind(const std::string& addr, int port)
{
    return details::Bind(m_async_context.get(), addr, port);
}


inline bool AsyncTcpSocket::Listen(int backlog)
{
    return details::Listen(m_async_context.get(), backlog);
}

inline GHandle AsyncTcpSocket::GetHandle() const
{
    return m_async_context->m_handle;
}

inline uint32_t AsyncTcpSocket::GetErrorCode() const
{
    return m_async_context->m_error_code;
}

template <typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncTcpSocket::Connect(THost *addr, int64_t timeout)
{
    return details::AsyncConnect<CoRtn>(m_async_context.get(), addr, timeout);
}

template <typename CoRtn>
inline AsyncResult<GHandle, CoRtn> AsyncTcpSocket::Accept(THost *addr, int64_t timeout)
{
    return details::AsyncAccept<CoRtn>(m_async_context.get(), addr, timeout);
}

template <typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncTcpSocket::Recv(TcpIOVecHolder& holder, size_t length, int64_t timeout)
{
    return details::AsyncRecv<CoRtn>(m_async_context.get(), &holder, length, timeout);
}

template <typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncTcpSocket::Send(TcpIOVecHolder& holder, size_t length, int64_t timeout)
{
    return details::AsyncSend<CoRtn>(m_async_context.get(), &holder, length, timeout);
}

template <typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncTcpSocket::Close()
{
    return details::AsyncNetClose<CoRtn>(m_async_context.get());
}


inline AsyncTcpSslSocket::AsyncTcpSslSocket()
    :m_async_context(AsyncSslNetEventContext::Create())
{
    if(!SslCtx) {
        SslCtx = GetGlobalSSLCtx();
    }
}


inline AsyncTcpSslSocket::AsyncTcpSslSocket(GHandle handle)
    :m_async_context(AsyncSslNetEventContext::Create(handle))
{
    if(!SslCtx) {
        SslCtx = GetGlobalSSLCtx();
    }
}


inline SSL_CTX* AsyncTcpSslSocket::SslCtx = nullptr;


inline AsyncTcpSslSocket::AsyncTcpSslSocket(SSL *ssl)
    :m_async_context(AsyncSslNetEventContext::Create(ssl))
{
    if(!SslCtx) {
        SslCtx = GetGlobalSSLCtx();
    }
}


inline bool AsyncTcpSslSocket::Socket(bool noblock) const
{
    if(!details::AsyncTcpSocket(m_async_context.get(), noblock)){
        return false;
    }
    return details::AsyncSSLSocket(m_async_context.get(), SslCtx);
}


inline bool AsyncTcpSslSocket::Socket(GHandle handle)
{
    m_async_context->m_handle = handle;
    HandleOption option(handle);
    if(!option.HandleNonBlock()) {
        return false;
    }
    return details::AsyncSSLSocket(m_async_context.get(), SslCtx);;
}


inline bool AsyncTcpSslSocket::Bind(const std::string& addr, int port)
{
    return details::Bind(m_async_context.get(), addr, port);
}


inline bool AsyncTcpSslSocket::Listen(int backlog)
{
    return details::Listen(m_async_context.get(), backlog);
}

template <typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncTcpSslSocket::Connect(THost *addr, int64_t timeout)
{
    return details::AsyncConnect<CoRtn>(m_async_context, addr, timeout);
}

template <typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncTcpSslSocket::AsyncSSLConnect(int64_t timeout)
{
    return details::AsyncSSLConnect<CoRtn>(m_async_context.get());
}

template <typename CoRtn>
inline AsyncResult<GHandle, CoRtn> AsyncTcpSslSocket::Accept(THost *addr, int64_t timeout)
{
    return details::AsyncAccept<CoRtn>(m_async_context.get(), addr, timeout);
}

template <typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncTcpSslSocket::SSLAccept(int64_t timeout)
{
    return details::AsyncSSLAccept<CoRtn>(m_async_context.get(), timeout);
}

template <typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncTcpSslSocket::Recv(TcpIOVecHolder& holder, size_t length, int64_t timeout)
{
    return details::AsyncSSLRecv<CoRtn>(m_async_context.get(), &holder, length, timeout);
}

template <typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncTcpSslSocket::Send(TcpIOVecHolder& holder, size_t length, int64_t timeout)
{
    return details::AsyncSSLSend<CoRtn>(m_async_context.get(), &holder, length, timeout);
}

template <typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncTcpSslSocket::Close()
{
    return details::AsyncSSLClose<CoRtn>(m_async_context.get());
}

inline GHandle AsyncTcpSslSocket::GetHandle() const
{
    return m_async_context->m_handle;
}

inline uint32_t AsyncTcpSslSocket::GetErrorCode() const
{
    return m_async_context->m_error_code;
}

inline AsyncUdpSocket::AsyncUdpSocket()
    :m_async_context(AsyncNetEventContext::Create())
{
}


inline AsyncUdpSocket::AsyncUdpSocket(GHandle handle)
    :m_async_context(AsyncNetEventContext::Create(handle))
{
}


inline bool AsyncUdpSocket::Socket(bool noblock) const
{
    return details::AsyncUdpSocket(m_async_context.get(), noblock);
}


inline bool AsyncUdpSocket::Bind(const std::string& addr, int port)
{
    return details::Bind(m_async_context.get(), addr, port);
}

inline GHandle AsyncUdpSocket::GetHandle() const
{
    return m_async_context->m_handle;
}

inline uint32_t AsyncUdpSocket::GetErrorCode() const
{
    return m_async_context->m_error_code;
}


template <typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncUdpSocket::RecvFrom(UdpIOVecHolder& holder, size_t length, int64_t timeout)
{
    return details::AsyncRecvFrom<CoRtn>(m_async_context.get(), &holder, length, timeout);
}

template <typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncUdpSocket::SendTo(UdpIOVecHolder& holder, size_t length, int64_t timeout)
{
    return details::AsyncSendTo<CoRtn>(m_async_context.get(), &holder, length, timeout);
}

template <typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncUdpSocket::Close()
{
    return details::AsyncNetClose<CoRtn>(m_async_context.get());
}


inline AsyncFileDescriptor::AsyncFileDescriptor()
    :m_async_context(AsyncFileEventContext::Create())
{
}


inline AsyncFileDescriptor::AsyncFileDescriptor(GHandle handle)
    :m_async_context(AsyncFileEventContext::Create(handle))
{
}


inline bool AsyncFileDescriptor::Open(const char *path, int flags, mode_t mode)
{
    return details::AsyncFileOpen(m_async_context.get(), path, flags, mode);
}

inline GHandle AsyncFileDescriptor::GetHandle() const
{
    return m_async_context->m_handle;
}

inline uint32_t AsyncFileDescriptor::GetErrorCode() const
{
    return m_async_context->m_error_code;
}


template <typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncFileDescriptor::Read(FileIOVecHolder& holder, size_t length, int64_t timeout)
{
    return details::AsyncFileRead<CoRtn>(m_async_context.get(), &holder, length, timeout);
}

template <typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncFileDescriptor::Write(FileIOVecHolder& holder, size_t length, int64_t timeout)
{
    return details::AsyncFileWrite<CoRtn>(m_async_context.get(), &holder, length, timeout);
}

template <typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncFileDescriptor::Close()
{
    return details::AsyncFileClose<CoRtn>(m_async_context.get());
}


#ifdef __linux__


inline AsyncFileNativeAioDescriptor::AsyncFileNativeAioDescriptor(int maxevents)
    :m_async_context(AsyncLinuxFileEventContext::Create(maxevents))
{
    m_async_context->m_event_handle.fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC | EFD_SEMAPHORE);
}


inline AsyncFileNativeAioDescriptor::AsyncFileNativeAioDescriptor(GHandle handle, int maxevents)
    :m_async_context(AsyncLinuxFileEventContext::Create(handle, maxevents))
{
    m_async_context->m_event_handle.fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC | EFD_SEMAPHORE);
}


inline bool AsyncFileNativeAioDescriptor::Open(const char *path, int flags, mode_t mode)
{
    return details::AsyncFileOpen(m_async_context.get(), path, flags, mode);
}


inline bool AsyncFileNativeAioDescriptor::PrepareRead(char *buf, size_t len, long long offset, AioCallback *callback)
{
    uint32_t num = m_async_context->m_current_index.load();
    if(num >= m_async_context->m_iocbs.size()) return false;
    if(!m_async_context->m_current_index.compare_exchange_strong(num, num + 1)) return false;
    if(!m_async_context->m_unfinished_io.compare_exchange_strong(num, num + 1)) return false;
    io_prep_pread(m_async_context->m_iocb_ptrs[num], m_async_context->m_handle.fd, buf, len, offset);
    io_set_eventfd(m_async_context->m_iocb_ptrs[num], m_async_context->m_event_handle.fd);
    m_async_context->m_iocb_ptrs[num]->data = callback;
    return true;
}


inline bool AsyncFileNativeAioDescriptor::PrepareWrite(char *buf, size_t len, long long offset, AioCallback *callback)
{
    uint32_t num = m_async_context->m_current_index.load();
    if(num >= m_async_context->m_iocbs.size()) return false;
    if(!m_async_context->m_current_index.compare_exchange_strong(num, num + 1)) return false;
    if(!m_async_context->m_unfinished_io.compare_exchange_strong(num, num + 1)) return false;
    io_prep_pwrite(m_async_context->m_iocb_ptrs[num], m_async_context->m_handle.fd, buf, len, offset);
    io_set_eventfd(m_async_context->m_iocb_ptrs[num], m_async_context->m_event_handle.fd);
    m_async_context->m_iocb_ptrs[num]->data = callback;
    return true;
}


inline bool AsyncFileNativeAioDescriptor::PrepareReadV(iovec *iov, int count, long long offset, AioCallback *callback)
{
    uint32_t num = m_async_context->m_current_index.load();
    if(num >= m_async_context->m_iocbs.size()) return false;
    if(!m_async_context->m_current_index.compare_exchange_strong(num, num + 1)) return false;
    if(!m_async_context->m_unfinished_io.compare_exchange_strong(num, num + 1)) return false;
    io_prep_preadv(m_async_context->m_iocb_ptrs[num], m_async_context->m_handle.fd, iov, count, offset);
    io_set_eventfd(m_async_context->m_iocb_ptrs[num], m_async_context->m_event_handle.fd);
    m_async_context->m_iocb_ptrs[num]->data = callback;
    return true;
}


inline bool AsyncFileNativeAioDescriptor::PrepareWriteV(iovec *iov, int count, long long offset, AioCallback *callback)
{
    uint32_t num = m_async_context->m_current_index.load();
    if(num >= m_async_context->m_iocbs.size()) return false;
    if(!m_async_context->m_current_index.compare_exchange_strong(num, num + 1)) return false;
    if(!m_async_context->m_unfinished_io.compare_exchange_strong(num, num + 1)) return false;
    io_prep_pwritev(m_async_context->m_iocb_ptrs[num], m_async_context->m_handle.fd, iov, count, offset);
    io_set_eventfd(m_async_context->m_iocb_ptrs[num], m_async_context->m_event_handle.fd);
    m_async_context->m_iocb_ptrs[num]->data = callback;
    return true;
}

template <typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncFileNativeAioDescriptor::Commit()
{
    int ret = io_submit(m_async_context->m_ioctx, m_async_context->m_unfinished_io, m_async_context->m_iocb_ptrs.data());
    if( ret < 0 ) {
        m_async_context->m_error_code = error::MakeErrorCode(error::ErrorCode::Error_LinuxAioSubmitError, -ret);
        return AsyncResult<int, CoRtn>(-1);
    }
    m_async_context->m_current_index = 0;
    static_cast<details::FileIoWaitEvent*>(m_async_context->m_action->GetBindEvent())->ResetFileIoWaitEventType(details::kFileIoWaitEventTypeLinuxAio);
    return {m_async_context->m_action.get(), nullptr};
}

template <typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncFileNativeAioDescriptor::Close()
{
    return details::AsyncFileClose<CoRtn>(m_async_context.get());
}


inline AsyncFileNativeAioDescriptor::~AsyncFileNativeAioDescriptor()
{
    if(m_async_context->m_event_handle.fd > 0) close(m_async_context->m_event_handle.fd);
}

#endif

}


#endif