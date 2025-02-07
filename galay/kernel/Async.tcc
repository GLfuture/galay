#ifndef GALAY_ASYNC_TCC
#define GALAY_ASYNC_TCC

#include "Async.hpp"
#if defined(__linux__)
    #include <sys/eventfd.h>
#endif

namespace galay::details
{
inline bool AsyncTcpSocket(AsyncNetEventContext* async_context)
{
    async_context->m_error_code = error::ErrorCode::Error_NoError;
    async_context->m_handle.fd = socket(AF_INET, SOCK_STREAM, 0);
    if (async_context->m_handle.fd < 0) {
        async_context->m_error_code = error::MakeErrorCode(error::ErrorCode::Error_SocketError, errno);
        return false;
    }
    HandleOption option(async_context->m_handle);
    option.HandleNonBlock();
    return true;
}


inline bool AsyncUdpSocket(AsyncNetEventContext* async_context)
{
    async_context->m_error_code = error::ErrorCode::Error_NoError;
    async_context->m_handle.fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (async_context->m_handle.fd < 0) {
        async_context->m_error_code = error::MakeErrorCode(error::ErrorCode::Error_SocketError, errno);
        return false;
    }
    HandleOption option(async_context->m_handle);
    option.HandleNonBlock();
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
    ////async_context->m_timeout = timeout;
    return {async_context->m_action, addr};
}

template<typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncConnect(AsyncNetEventContext* async_context, THost* addr, int64_t timeout)
{
    static_cast<NetWaitEvent*>(async_context->m_action->GetBindEvent())->ResetNetWaitEventType(kTcpWaitEventTypeConnect);
    async_context->m_error_code = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    ////async_context->m_timeout = timeout;
    return {async_context->m_action, addr};
}

template<typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncRecv(AsyncNetEventContext* async_context, TcpIOVec* iov, size_t length, int64_t timeout)
{
    static_cast<NetWaitEvent*>(async_context->m_action->GetBindEvent())->ResetNetWaitEventType(kTcpWaitEventTypeRecv);
    async_context->m_error_code = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    //async_context->m_timeout = timeout;
    iov->m_length = length;
    return {async_context->m_action, iov};
}

template<typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncSend(AsyncNetEventContext* async_context, TcpIOVec *iov, size_t length, int64_t timeout)
{
    static_cast<NetWaitEvent*>(async_context->m_action->GetBindEvent())->ResetNetWaitEventType(kTcpWaitEventTypeSend);
    async_context->m_error_code = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    //async_context->m_timeout = timeout;
    iov->m_length = length;
    return {async_context->m_action, iov};
}

template<typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncRecvFrom(AsyncNetEventContext* async_context, UdpIOVec *iov, size_t length, int64_t timeout)
{
    static_cast<NetWaitEvent*>(async_context->m_action->GetBindEvent())->ResetNetWaitEventType(kUdpWaitEventTypeRecvFrom);
    async_context->m_error_code = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    //async_context->m_timeout = timeout;
    iov->m_length = length;
    return {async_context->m_action, iov};
}

template<typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncSendTo(AsyncNetEventContext* async_context, UdpIOVec *iov, size_t length, int64_t timeout)
{
    static_cast<NetWaitEvent*>(async_context->m_action->GetBindEvent())->ResetNetWaitEventType(kUdpWaitEventTypeSendTo);
    async_context->m_error_code = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    //async_context->m_timeout = timeout;
    iov->m_length = length;
    return {async_context->m_action, iov};
}

template<typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncNetClose(AsyncNetEventContext* async_context)
{
    static_cast<NetWaitEvent*>(async_context->m_action->GetBindEvent())->ResetNetWaitEventType(kWaitEventTypeClose);
    async_context->m_error_code = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return {async_context->m_action, nullptr};
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
    //async_context->m_timeout = timeout;
    return {async_context->m_action, nullptr};
}

template<typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncSSLConnect(AsyncSslNetEventContext* async_context, int64_t timeout)
{
    static_cast<NetWaitEvent*>(async_context->m_action->GetBindEvent())->ResetNetWaitEventType(kTcpWaitEventTypeSslConnect);
    async_context->m_error_code = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    //async_context->m_timeout = timeout;
    return {async_context->m_action, nullptr};
}

template<typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncSSLRecv(AsyncSslNetEventContext* async_context, TcpIOVec *iov, size_t length, int64_t timeout)
{
    static_cast<NetWaitEvent*>(async_context->m_action->GetBindEvent())->ResetNetWaitEventType(kTcpWaitEventTypeSslRecv);
    async_context->m_error_code = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    //async_context->m_timeout = timeout;
    iov->m_length = length;
    return {async_context->m_action, iov};
}

template<typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncSSLSend(AsyncSslNetEventContext* async_context, TcpIOVec *iov, size_t length, int64_t timeout)
{
    static_cast<NetWaitEvent*>(async_context->m_action->GetBindEvent())->ResetNetWaitEventType(kTcpWaitEventTypeSslSend);
    async_context->m_error_code = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    //async_context->m_timeout = timeout;
    iov->m_length = length;
    return {async_context->m_action, iov};
}

template<typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncSSLClose(AsyncSslNetEventContext* async_context)
{
    static_cast<NetWaitEvent*>(async_context->m_action->GetBindEvent())->ResetNetWaitEventType(kWaitEventTypeSslClose);
    async_context->m_error_code = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return {async_context->m_action, nullptr};
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
    //async_context->m_timeout = timeout;
    iov->m_length = length;
    return {async_context->m_action, iov};
}

template<typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncFileWrite(AsyncFileEventContext* async_context, FileIOVec *iov, size_t length, int64_t timeout)
{
    static_cast<FileIoWaitEvent*>(async_context->m_action->GetBindEvent())->ResetFileIoWaitEventType(kFileIoWaitEventTypeWrite);
    async_context->m_error_code = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    //async_context->m_timeout = timeout;
    iov->m_length = length;
    return {async_context->m_action, iov};
}

template<typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncFileClose(AsyncFileEventContext* async_context)
{
    static_cast<FileIoWaitEvent*>(async_context->m_action->GetBindEvent())->ResetFileIoWaitEventType(kFileIoWaitEventTypeClose);
    async_context->m_error_code = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return {async_context->m_action, nullptr};
}

}


namespace galay
{


inline AsyncNetEventContext* AsyncNetEventContext::Create(details::EventScheduler* scheduler)
{
    auto context = new AsyncNetEventContext;
    context->m_action = new details::IOEventAction(scheduler->GetEngine(), new details::NetWaitEvent(context));
    context->m_ticker = std::make_shared<AsyncTimeoutTicker>();
    return context;
}

inline AsyncNetEventContext* AsyncNetEventContext::Create(GHandle handle, details::EventScheduler* scheduler)
{
    auto context = new AsyncNetEventContext;
    context->m_action = new details::IOEventAction(scheduler->GetEngine(), new details::NetWaitEvent(context));
    context->m_ticker = std::make_shared<AsyncTimeoutTicker>();
    context->m_handle = handle;
    return context;
}

inline AsyncNetEventContext::~AsyncNetEventContext()
{
    if(m_action) {
        delete m_action;
        m_action = nullptr;
    }
}


inline AsyncSslNetEventContext* AsyncSslNetEventContext::Create(details::EventScheduler* scheduler)
{
    auto context = new AsyncSslNetEventContext;
    context->m_action = new details::IOEventAction(scheduler->GetEngine(), new details::NetSslWaitEvent(context));
    return context;
}

inline AsyncSslNetEventContext* AsyncSslNetEventContext::Create(GHandle handle, details::EventScheduler* scheduler)
{
    auto context = new AsyncSslNetEventContext;
    context->m_action = new details::IOEventAction(scheduler->GetEngine(), new details::NetSslWaitEvent(context));
    context->m_handle = handle;
    return context;
}

inline AsyncSslNetEventContext* AsyncSslNetEventContext::Create(SSL *ssl, details::EventScheduler* scheduler)
{
    auto context = new AsyncSslNetEventContext;
    context->m_action = new details::IOEventAction(scheduler->GetEngine(), new details::NetSslWaitEvent(context));
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

inline AsyncFileEventContext* galay::AsyncFileEventContext::Create(details::EventScheduler* scheduler)
{
    auto context = new AsyncFileEventContext;
    context->m_action = new details::IOEventAction(scheduler->GetEngine(), new details::FileIoWaitEvent(context));
    return context;
}

inline AsyncFileEventContext* AsyncFileEventContext::Create(GHandle handle, details::EventScheduler* scheduler)
{
    auto context = new AsyncFileEventContext;
    context->m_action = new details::IOEventAction(scheduler->GetEngine(), new details::FileIoWaitEvent(context));
    context->m_handle = handle;
    return context;
}

inline AsyncFileEventContext::~AsyncFileEventContext()
{
    delete m_action;
}


#ifdef __linux__

inline AsyncLinuxFileEventContext* AsyncLinuxFileEventContext::Create(details::EventScheduler* scheduler, int maxevents)
{
    return new AsyncLinuxFileEventContext(scheduler, maxevents);
}

inline AsyncLinuxFileEventContext* AsyncLinuxFileEventContext::Create(GHandle handle, details::EventScheduler* scheduler, int maxevents)
{
    return new AsyncLinuxFileEventContext(handle, scheduler, maxevents);
}

inline AsyncLinuxFileEventContext::AsyncLinuxFileEventContext(details::EventScheduler* scheduler, int maxevents)
    : AsyncFileEventContext(scheduler), m_current_index(0), m_unfinished_io(0)
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


inline AsyncLinuxFileEventContext::AsyncLinuxFileEventContext(GHandle handle, details::EventScheduler* scheduler, int maxevents)
    :AsyncFileEventContext(handle, scheduler), m_current_index(0), m_unfinished_io(0)
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


inline bool AsyncLinuxFileEventContext::InitialEventHandle()
{
    m_event_handle.fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC | EFD_SEMAPHORE);
    if(m_event_handle.fd < 0) return false;
    return true;
}


inline bool AsyncLinuxFileEventContext::InitialEventHandle(GHandle handle)
{
    m_event_handle.fd = handle.fd;
    return true;
}


inline bool AsyncLinuxFileEventContext::CloseEventHandle()
{
    int ret = close(m_event_handle.fd);
    if(ret < 0) return false;
    m_event_handle.fd = 0;
    return true;
}


inline bool AsyncLinuxFileEventContext::PrepareRead(char *buf, size_t len, long long offset, AioCallback* callback)
{
    uint32_t num = this->m_current_index.load();
    if(num >= this->m_iocbs.size()) return false;
    if(!this->m_current_index.compare_exchange_strong(num, num + 1)) return false;
    if(!this->m_unfinished_io.compare_exchange_strong(num, num + 1)) return false;
    io_prep_pread(this->m_iocb_ptrs[num], this->m_handle.fd, buf, len, offset);
    io_set_eventfd(this->m_iocb_ptrs[num], this->m_event_handle.fd);
    this->m_iocb_ptrs[num]->data = callback;
    return true;
}


inline bool AsyncLinuxFileEventContext::PrepareWrite(char *buf, size_t len, long long offset, AioCallback* callback)
{
    uint32_t num = this->m_current_index.load();
    if(num >= this->m_iocbs.size()) return false;
    if(!this->m_current_index.compare_exchange_strong(num, num + 1)) return false;
    if(!this->m_unfinished_io.compare_exchange_strong(num, num + 1)) return false;
    io_prep_pwrite(this->m_iocb_ptrs[num], this->m_handle.fd, buf, len, offset);
    io_set_eventfd(this->m_iocb_ptrs[num], this->m_event_handle.fd);
    this->m_iocb_ptrs[num]->data = callback;
    return true;
}


inline bool AsyncLinuxFileEventContext::PrepareReadV(iovec* iov, int count, long long offset, AioCallback* callback)
{
    uint32_t num = this->m_current_index.load();
    if(num >= this->m_iocbs.size()) return false;
    if(!this->m_current_index.compare_exchange_strong(num, num + 1)) return false;
    if(!this->m_unfinished_io.compare_exchange_strong(num, num + 1)) return false;
    io_prep_preadv(this->m_iocb_ptrs[num], this->m_handle.fd, iov, count, offset);
    io_set_eventfd(this->m_iocb_ptrs[num], this->m_event_handle.fd);
    this->m_iocb_ptrs[num]->data = callback;
    return true;
}


inline bool AsyncLinuxFileEventContext::PrepareWriteV(iovec* iov, int count, long long offset, AioCallback* callback)
{
    uint32_t num = this->m_current_index.load();
    if(num >= this->m_iocbs.size()) return false;
    if(!this->m_current_index.compare_exchange_strong(num, num + 1)) return false;
    if(!this->m_unfinished_io.compare_exchange_strong(num, num + 1)) return false;
    io_prep_pwritev(this->m_iocb_ptrs[num], this->m_handle.fd, iov, count, offset);
    io_set_eventfd(this->m_iocb_ptrs[num], this->m_event_handle.fd);
    this->m_iocb_ptrs[num]->data = callback;
    return true;
}

template <typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncLinuxFileEventContext::Commit()
{
    int ret = io_submit(this->m_ioctx, this->m_unfinished_io, this->m_iocb_ptrs.data());
    if( ret < 0 ) {
        this->m_error_code = error::MakeErrorCode(error::ErrorCode::Error_LinuxAioSubmitError, -ret);
        return AsyncResult<int, CoRtn>(-1);
    }
    this->m_current_index = 0;
    static_cast<details::FileIoWaitEvent*>(this->m_action->GetBindEvent())->ResetFileIoWaitEventType(details::kFileIoWaitEventTypeLinuxAio);
    return {this->m_action, nullptr};
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



inline AsyncTcpSocket::AsyncTcpSocket(details::EventScheduler* scheduler)
    :m_async_context(AsyncNetEventContext::Create(scheduler))
{
}


inline AsyncTcpSocket::AsyncTcpSocket(GHandle handle, details::EventScheduler *scheduler)
    :m_async_context(AsyncNetEventContext::Create(handle, scheduler))
{
}


inline bool AsyncTcpSocket::Socket() const
{
    return details::AsyncTcpSocket(m_async_context);
}


inline bool AsyncTcpSocket::Socket(GHandle handle)
{
    m_async_context->m_handle = handle;
    HandleOption option(handle);
    return option.HandleNonBlock();
}


inline bool AsyncTcpSocket::Bind(const std::string& addr, int port)
{
    return details::Bind(m_async_context, addr, port);
}


inline bool AsyncTcpSocket::Listen(int backlog)
{
    return details::Listen(m_async_context, backlog);
}

inline GHandle AsyncTcpSocket::GetHandle() const
{
    return m_async_context->m_handle;
}

inline uint32_t AsyncTcpSocket::GetErrorCode() const
{
    return m_async_context->m_error_code;
}

inline AsyncTcpSocket::~AsyncTcpSocket()
{
    if(m_async_context) {
        delete m_async_context;
        m_async_context = nullptr;
    }
}

template <typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncTcpSocket::Connect(THost *addr, int64_t timeout)
{
    return details::AsyncConnect<CoRtn>(m_async_context, addr, timeout);
}

template <typename CoRtn>
inline AsyncResult<GHandle, CoRtn> AsyncTcpSocket::Accept(THost *addr, int64_t timeout)
{
    return details::AsyncAccept<CoRtn>(m_async_context, addr, timeout);
}

template <typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncTcpSocket::Recv(TcpIOVec *iov, size_t length, int64_t timeout)
{
    return details::AsyncRecv<CoRtn>(m_async_context, iov, length, timeout);
}

template <typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncTcpSocket::Send(TcpIOVec *iov, size_t length, int64_t timeout)
{
    return details::AsyncSend<CoRtn>(m_async_context, iov, length, timeout);
}

template <typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncTcpSocket::Close()
{
    return details::AsyncNetClose<CoRtn>(m_async_context);
}


inline AsyncTcpSslSocket::AsyncTcpSslSocket(details::EventScheduler* scheduler)
    :m_async_context(AsyncSslNetEventContext::Create(scheduler))
{
    if(!SslCtx) {
        SslCtx = GetGlobalSSLCtx();
    }
}


inline AsyncTcpSslSocket::AsyncTcpSslSocket(GHandle handle, details::EventScheduler* scheduler)
    :m_async_context(AsyncSslNetEventContext::Create(handle, scheduler))
{
    if(!SslCtx) {
        SslCtx = GetGlobalSSLCtx();
    }
}


inline SSL_CTX* AsyncTcpSslSocket::SslCtx = nullptr;


inline AsyncTcpSslSocket::AsyncTcpSslSocket(SSL *ssl, details::EventScheduler* scheduler)
    :m_async_context(AsyncSslNetEventContext::Create(ssl, scheduler))
{
    if(!SslCtx) {
        SslCtx = GetGlobalSSLCtx();
    }
}


inline bool AsyncTcpSslSocket::Socket() const
{
    if(!details::AsyncTcpSocket(m_async_context)){
        return false;
    }
    return details::AsyncSSLSocket(m_async_context, SslCtx);
}


inline bool AsyncTcpSslSocket::Socket(GHandle handle)
{
    m_async_context->m_handle = handle;
    HandleOption option(handle);
    if(!option.HandleNonBlock()) {
        return false;
    }
    return details::AsyncSSLSocket(m_async_context, SslCtx);;
}


inline bool AsyncTcpSslSocket::Bind(const std::string& addr, int port)
{
    return details::Bind(m_async_context, addr, port);
}


inline bool AsyncTcpSslSocket::Listen(int backlog)
{
    return details::Listen(m_async_context, backlog);
}

template <typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncTcpSslSocket::Connect(THost *addr, int64_t timeout)
{
    return details::AsyncConnect<CoRtn>(m_async_context, addr, timeout);
}

template <typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncTcpSslSocket::AsyncSSLConnect(int64_t timeout)
{
    return details::AsyncSSLConnect<CoRtn>(m_async_context);
}

template <typename CoRtn>
inline AsyncResult<GHandle, CoRtn> AsyncTcpSslSocket::Accept(THost *addr, int64_t timeout)
{
    return details::AsyncAccept<CoRtn>(m_async_context, addr, timeout);
}

template <typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncTcpSslSocket::SSLAccept(int64_t timeout)
{
    return details::AsyncSSLAccept<CoRtn>(m_async_context, timeout);
}

template <typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncTcpSslSocket::Recv(TcpIOVec *iov, size_t length, int64_t timeout)
{
    return details::AsyncSSLRecv<CoRtn>(m_async_context, iov, length, timeout);
}

template <typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncTcpSslSocket::Send(TcpIOVec *iov, size_t length, int64_t timeout)
{
    return details::AsyncSSLSend<CoRtn>(m_async_context, iov, length, timeout);
}

template <typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncTcpSslSocket::Close()
{
    return details::AsyncSSLClose<CoRtn>(m_async_context);
}

inline GHandle AsyncTcpSslSocket::GetHandle() const
{
    return m_async_context->m_handle;
}

inline uint32_t AsyncTcpSslSocket::GetErrorCode() const
{
    return m_async_context->m_error_code;
}

inline AsyncUdpSocket::AsyncUdpSocket(details::EventScheduler* scheduler)
    :m_async_context(AsyncNetEventContext::Create(scheduler))
{
}


inline AsyncUdpSocket::AsyncUdpSocket(GHandle handle, details::EventScheduler* scheduler)
    :m_async_context(AsyncNetEventContext::Create(handle, scheduler))
{
}


inline bool AsyncUdpSocket::Socket() const
{
    return details::AsyncUdpSocket(m_async_context);
}


inline bool AsyncUdpSocket::Bind(const std::string& addr, int port)
{
    return details::Bind(m_async_context, addr, port);
}

inline GHandle AsyncUdpSocket::GetHandle() const
{
    return m_async_context->m_handle;
}

inline uint32_t AsyncUdpSocket::GetErrorCode() const
{
    return m_async_context->m_error_code;
}

inline AsyncUdpSocket::~AsyncUdpSocket()
{
    if(m_async_context) {
        delete m_async_context;
        m_async_context = nullptr;
    }
}

template <typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncUdpSocket::RecvFrom(UdpIOVec *iov, size_t length, int64_t timeout)
{
    return details::AsyncRecvFrom<CoRtn>(m_async_context, iov, length, timeout);
}

template <typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncUdpSocket::SendTo(UdpIOVec *iov, size_t length, int64_t timeout)
{
    return details::AsyncSendTo<CoRtn>(m_async_context, iov, length, timeout);
}

template <typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncUdpSocket::Close()
{
    return details::AsyncNetClose<CoRtn>(m_async_context);
}


inline AsyncFileDescriptor::AsyncFileDescriptor(details::EventScheduler* scheduler)
    :m_async_context(AsyncFileEventContext::Create(scheduler))
{
}


inline AsyncFileDescriptor::AsyncFileDescriptor(GHandle handle, details::EventScheduler* scheduler)
    :m_async_context(AsyncFileEventContext::Create(handle, scheduler))
{
}


inline bool AsyncFileDescriptor::Open(const char *path, int flags, mode_t mode)
{
    return details::AsyncFileOpen(m_async_context, path, flags, mode);
}

inline GHandle AsyncFileDescriptor::GetHandle() const
{
    return m_async_context->m_handle;
}

inline uint32_t AsyncFileDescriptor::GetErrorCode() const
{
    return m_async_context->m_error_code;
}

inline AsyncFileDescriptor::~AsyncFileDescriptor()
{
    if( m_async_context ) {
        delete m_async_context;
        m_async_context = nullptr;
    }
}

template <typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncFileDescriptor::Read(FileIOVec* iov, size_t length, int64_t timeout)
{
    return details::AsyncFileRead<CoRtn>(m_async_context, iov, length, timeout);
}

template <typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncFileDescriptor::Write(FileIOVec* iov, size_t length, int64_t timeout)
{
    return details::AsyncFileWrite<CoRtn>(m_async_context, iov, length, timeout);
}

template <typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncFileDescriptor::Close()
{
    return details::AsyncFileClose<CoRtn>(m_async_context);
}


#ifdef __linux__


inline AsyncFileNativeAioDescriptor::AsyncFileNativeAioDescriptor(details::EventScheduler* scheduler, int maxevents)
    :m_async_context(AsyncLinuxFileEventContext::Create(engine, maxevents))
{
    m_async_context->InitialEventHandle();
}


inline AsyncFileNativeAioDescriptor::AsyncFileNativeAioDescriptor(GHandle handle, details::EventScheduler* scheduler, int maxevents)
    :m_async_context(AsyncLinuxFileEventContext::Create(handle, engine, maxevents))
{
    m_async_context->InitialEventHandle();
}


inline bool AsyncFileNativeAioDescriptor::Open(const char *path, int flags, mode_t mode)
{
    return details::AsyncFileOpen(m_async_context, path, flags, mode);
}


inline bool AsyncFileNativeAioDescriptor::PrepareRead(char *buf, size_t len, long long offset, AioCallback *callback)
{
    return m_async_context->PrepareRead(buf, len, offset, callback);
}


inline bool AsyncFileNativeAioDescriptor::PrepareWrite(char *buf, size_t len, long long offset, AioCallback *callback)
{
    return m_async_context->PrepareWrite(buf, len, offset, callback);
}


inline bool AsyncFileNativeAioDescriptor::PrepareReadV(iovec *iov, int count, long long offset, AioCallback *callback)
{
    return m_async_context->PrepareReadV(iov, count, offset, callback);
}


inline bool AsyncFileNativeAioDescriptor::PrepareWriteV(iovec *iov, int count, long long offset, AioCallback *callback)
{
    return m_async_context->PrepareWriteV(iov, count, offset, callback);
}

template <typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncFileNativeAioDescriptor::Commit()
{
    return m_async_context->Commit<CoRtn>();
}

template <typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncFileNativeAioDescriptor::Close()
{
    return details::AsyncFileClose<CoRtn>(m_async_context);
}


inline AsyncFileNativeAioDescriptor::~AsyncFileNativeAioDescriptor()
{
    m_async_context->CloseEventHandle();
    if( m_async_context ) {
        delete m_async_context;
        m_async_context = nullptr;
    }
}

#endif

}


#endif