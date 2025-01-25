#ifndef GALAY_ASYNC_TCC
#define GALAY_ASYNC_TCC

#include "Async.hpp"
#if defined(__linux__)
    #include <sys/eventfd.h>
#endif

namespace galay::details
{
inline bool AsyncTcpSocket(AsyncNetIo::wptr aio)
{
    if (aio.expired()) {
        return false;
    }
    aio.lock()->GetErrorCode() = error::ErrorCode::Error_NoError;
    aio.lock()->GetHandle().fd = socket(AF_INET, SOCK_STREAM, 0);
    if (aio.lock()->GetHandle().fd < 0) {
        aio.lock()->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_SocketError, errno);
        return false;
    }
    HandleOption option(aio.lock()->GetHandle());
    option.HandleNonBlock();
    return true;
}


inline bool AsyncUdpSocket(AsyncNetIo::wptr aio)
{
    aio.lock()->GetErrorCode() = error::ErrorCode::Error_NoError;
    aio.lock()->GetHandle().fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (aio.lock()->GetHandle().fd < 0) {
        aio.lock()->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_SocketError, errno);
        return false;
    }
    HandleOption option(aio.lock()->GetHandle());
    option.HandleNonBlock();
    return true;
}


inline bool Bind(AsyncNetIo::wptr aio, const std::string& addr, int port)
{
    aio.lock()->GetErrorCode() = error::ErrorCode::Error_NoError;
    sockaddr_in taddr{};
    taddr.sin_family = AF_INET;
    taddr.sin_port = htons(port);
    if(addr.empty()) taddr.sin_addr.s_addr = INADDR_ANY;
    else taddr.sin_addr.s_addr = inet_addr(addr.c_str());
    if( bind(aio.lock()->GetHandle().fd, reinterpret_cast<sockaddr*>(&taddr), sizeof(taddr)) )
    {
        aio.lock()->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_BindError, errno);
        return false;
    }
    return true;
}


inline bool Listen(AsyncNetIo::wptr aio, int backlog)
{
    aio.lock()->GetErrorCode() = error::ErrorCode::Error_NoError;
    if( listen(aio.lock()->GetHandle().fd, backlog) )
    {
        aio.lock()->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_ListenError, errno);
        return false;
    }
    return true;
}

template<typename CoRtn>
inline AsyncResult<GHandle, CoRtn> AsyncAccept(AsyncNetIo::wptr aio, THost* addr)
{
    dynamic_cast<NetWaitEvent*>(aio.lock()->GetIOAction()->GetBindEvent())->ResetNetWaitEventType(kTcpWaitEventTypeAccept);
    aio.lock()->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return {aio.lock()->GetIOAction(), addr};
}

template<typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncConnect(AsyncNetIo::wptr aio, THost* addr)
{
    dynamic_cast<NetWaitEvent*>(aio.lock()->GetIOAction()->GetBindEvent())->ResetNetWaitEventType(kTcpWaitEventTypeConnect);
    aio.lock()->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return {aio.lock()->GetIOAction(), addr};
}

template<typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncRecv(AsyncNetIo::wptr aio, TcpIOVec* iov, size_t length)
{
    dynamic_cast<NetWaitEvent*>(aio.lock()->GetIOAction()->GetBindEvent())->ResetNetWaitEventType(kTcpWaitEventTypeRecv);
    aio.lock()->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    iov->m_length = length;
    return {aio.lock()->GetIOAction(), iov};
}

template<typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncSend(AsyncNetIo::wptr aio, TcpIOVec *iov, size_t length)
{
    dynamic_cast<NetWaitEvent*>(aio.lock()->GetIOAction()->GetBindEvent())->ResetNetWaitEventType(kTcpWaitEventTypeSend);
    aio.lock()->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    iov->m_length = length;
    return {aio.lock()->GetIOAction(), iov};
}

template<typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncRecvFrom(AsyncNetIo::wptr aio, UdpIOVec *iov, size_t length)
{
    dynamic_cast<NetWaitEvent*>(aio.lock()->GetIOAction()->GetBindEvent())->ResetNetWaitEventType(kUdpWaitEventTypeRecvFrom);
    aio.lock()->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    iov->m_length = length;
    return {aio.lock()->GetIOAction(), iov};
}

template<typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncSendTo(AsyncNetIo::wptr aio, UdpIOVec *iov, size_t length)
{
    dynamic_cast<NetWaitEvent*>(aio.lock()->GetIOAction()->GetBindEvent())->ResetNetWaitEventType(kUdpWaitEventTypeSendTo);
    aio.lock()->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    iov->m_length = length;
    return {aio.lock()->GetIOAction(), iov};
}

template<typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncNetClose(AsyncNetIo::wptr aio)
{
    dynamic_cast<NetWaitEvent*>(aio.lock()->GetIOAction()->GetBindEvent())->ResetNetWaitEventType(kWaitEventTypeClose);
    aio.lock()->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return {aio.lock()->GetIOAction(), nullptr};
}

inline bool AsyncSSLSocket(AsyncSslNetIo::wptr aio, SSL_CTX *ctx)
{
    SSL* ssl = SSL_new(ctx);
    if(ssl == nullptr) return false;
    if(SSL_set_fd(ssl, aio.lock()->GetHandle().fd)) {
        aio.lock()->GetSSL() = ssl;
        return true;
    }
    SSL_free(ssl);
    return false;
}

template<typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncSSLAccept(typename AsyncSslNetIo::wptr aio)
{
    dynamic_cast<NetWaitEvent*>(aio.lock()->GetIOAction()->GetBindEvent())->ResetNetWaitEventType(kTcpWaitEventTypeSslAccept);
    aio.lock()->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return {aio.lock()->GetIOAction(), nullptr};
}

template<typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncSSLConnect(typename AsyncSslNetIo::wptr aio)
{
    dynamic_cast<NetWaitEvent*>(aio.lock()->GetIOAction()->GetBindEvent())->ResetNetWaitEventType(kTcpWaitEventTypeSslConnect);
    aio.lock()->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return {aio.lock()->GetIOAction(), nullptr};
}

template<typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncSSLRecv(typename AsyncSslNetIo::wptr aio, TcpIOVec *iov, size_t length)
{
    dynamic_cast<NetWaitEvent*>(aio.lock()->GetIOAction()->GetBindEvent())->ResetNetWaitEventType(kTcpWaitEventTypeSslRecv);
    aio.lock()->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    iov->m_length = length;
    return {aio.lock()->GetIOAction(), iov};
}

template<typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncSSLSend(typename AsyncSslNetIo::wptr aio, TcpIOVec *iov, size_t length)
{
    dynamic_cast<NetWaitEvent*>(aio.lock()->GetIOAction()->GetBindEvent())->ResetNetWaitEventType(kTcpWaitEventTypeSslSend);
    aio.lock()->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    iov->m_length = length;
    return {aio.lock()->GetIOAction(), iov};
}

template<typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncSSLClose(typename AsyncSslNetIo::wptr aio)
{
    dynamic_cast<NetWaitEvent*>(aio.lock()->GetIOAction()->GetBindEvent())->ResetNetWaitEventType(kWaitEventTypeSslClose);
    aio.lock()->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return {aio.lock()->GetIOAction(), nullptr};
}


inline bool AsyncFileOpen(typename AsyncFileIo::wptr afile, const char *path, const int flags, mode_t mode)
{
#if defined(__linux__) || defined(__APPLE__) 
    const int fd = open(path, flags, mode);
    if( fd < 0 ) {
        return false;
    }
#endif
    afile.lock()->GetHandle() = GHandle{fd};
    HandleOption option(GHandle{fd});
    option.HandleNonBlock();
    return true;
}

template<typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncFileRead(typename AsyncFileIo::wptr afile, FileIOVec *iov,  size_t length)
{
    dynamic_cast<FileIoWaitEvent*>(afile.lock()->GetIOAction()->GetBindEvent())->ResetFileIoWaitEventType(kFileIoWaitEventTypeRead);
    afile.lock()->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    iov->m_length = length;
    return {afile.lock()->GetIOAction(), iov};
}

template<typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncFileWrite(typename AsyncFileIo::wptr afile, FileIOVec *iov, size_t length)
{
    dynamic_cast<FileIoWaitEvent*>(afile.lock()->GetIOAction()->GetBindEvent())->ResetFileIoWaitEventType(kFileIoWaitEventTypeWrite);
    afile.lock()->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    iov->m_length = length;
    return {afile.lock()->GetIOAction(), iov};
}

template<typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncFileClose(AsyncFileIo::wptr afile)
{
    dynamic_cast<FileIoWaitEvent*>(afile.lock()->GetIOAction()->GetBindEvent())->ResetFileIoWaitEventType(kFileIoWaitEventTypeClose);
    afile.lock()->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return {afile.lock()->GetIOAction(), nullptr};
}

}


namespace galay
{

inline AsyncNetIo::AsyncNetIo(details::EventEngine* engine)
    : m_handle({}), m_err_code(0), m_timer(nullptr)\
        , m_action(new details::IOEventAction(engine, new details::NetWaitEvent(this)))
{
}


inline AsyncNetIo::AsyncNetIo(GHandle handle, details::EventEngine *engine)
    :m_handle(handle), m_err_code(0), m_timer(nullptr)
{
    ActionInit(engine);
}


inline HandleOption
AsyncNetIo::GetOption() const
{
    return HandleOption(m_handle);
}

inline GHandle& AsyncNetIo::GetHandle() 
{ 
    return m_handle; 
}

inline details::IOEventAction* AsyncNetIo::GetIOAction() 
{ 
    return m_action; 
}

inline AsyncNetIo::~AsyncNetIo()
{
    delete m_action;
}


inline void AsyncNetIo::ActionInit(details::EventEngine *engine)
{
    m_action = new details::IOEventAction(engine, new details::NetWaitEvent(this));
}


inline uint32_t &AsyncNetIo::GetErrorCode()
{
    return m_err_code;
}


inline AsyncSslNetIo::AsyncSslNetIo(details::EventEngine *engine)
    : AsyncNetIo(engine), m_ssl(nullptr)
{
    ActionInit(engine);
}


inline AsyncSslNetIo::AsyncSslNetIo(GHandle handle, details::EventEngine *engine)
    :AsyncNetIo(handle, engine), m_ssl(nullptr)
{
}


inline AsyncSslNetIo::AsyncSslNetIo(SSL *ssl, details::EventEngine *engine)
    :AsyncNetIo({SSL_get_fd(ssl)}, engine), m_ssl(ssl)
{
}


inline AsyncSslNetIo::~AsyncSslNetIo()
{
    if(m_ssl) {
        SSL_shutdown(m_ssl);
        SSL_free(m_ssl);
    }
}


inline void AsyncSslNetIo::ActionInit(details::EventEngine *engine)
{
    this->m_action = new details::IOEventAction(engine, new details::NetSslWaitEvent(this));
}


inline AsyncFileIo::AsyncFileIo(details::EventEngine* engine)
    :m_handle({}), m_error_code(0), m_timer(nullptr)\
        , m_action(new details::IOEventAction(engine, new details::FileIoWaitEvent(this)))
{

}


inline AsyncFileIo::AsyncFileIo(GHandle handle, details::EventEngine *engine)
    :m_handle(handle), m_error_code(0), m_timer(nullptr)\
        , m_action(new details::IOEventAction(engine, new details::FileIoWaitEvent(this)))
{
}



inline HandleOption AsyncFileIo::GetOption() const
{
    return HandleOption(m_handle);
}

inline details::IOEventAction* AsyncFileIo::GetIOAction() 
{ 
    return m_action; 
}


inline GHandle& AsyncFileIo::GetHandle() 
{ 
    return m_handle; 
}

inline uint32_t& AsyncFileIo::GetErrorCode() 
{ 
    return m_error_code; 
}

inline AsyncFileIo::~AsyncFileIo()
{
    delete m_action;
}


#ifdef __linux__ 


inline AsyncFileNativeAio::AsyncFileNativeAio(details::EventEngine *engine, int maxevents)
    :AsyncFileIo(engine), m_current_index(0), m_unfinished_io(0)
{
    int ret = io_setup(maxevents, &m_ioctx);
    if(ret) {
        this->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_LinuxAioSetupError, errno);
        return;
    }
    m_iocbs.resize(maxevents);
    m_iocb_ptrs.resize(maxevents); 
    for (int i = 0 ; i < maxevents; ++i) {
        m_iocb_ptrs[i] = &m_iocbs[i];   
    }
}


inline AsyncFileNativeAio::AsyncFileNativeAio(GHandle handle, details::EventEngine *engine, int maxevents)
    :AsyncFileIo(handle, engine), m_current_index(0), m_unfinished_io(0)
{
    int ret = io_setup(maxevents, &m_ioctx);
    if(ret) {
        this->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_LinuxAioSetupError, errno);
        return;
    }
    m_iocbs.resize(maxevents);
    m_iocb_ptrs.resize(maxevents); 
    for (int i = 0 ; i < maxevents; ++i) {
        m_iocb_ptrs[i] = &m_iocbs[i];   
    }
}


inline bool AsyncFileNativeAio::InitialEventHandle()
{
    m_event_handle.fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC | EFD_SEMAPHORE);
    if(m_event_handle.fd < 0) return false;
    return true;
}


inline bool AsyncFileNativeAio::InitialEventHandle(GHandle handle)
{
    m_event_handle.fd = handle.fd;
    return true;
}


inline bool AsyncFileNativeAio::CloseEventHandle()
{
    int ret = close(m_event_handle.fd);
    if(ret < 0) return false;
    m_event_handle.fd = 0;
    return true;
}


inline bool AsyncFileNativeAio::PrepareRead(char *buf, size_t len, long long offset, AioCallback* callback)
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


inline bool AsyncFileNativeAio::PrepareWrite(char *buf, size_t len, long long offset, AioCallback* callback)
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


inline bool AsyncFileNativeAio::PrepareReadV(iovec* iov, int count, long long offset, AioCallback* callback)
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


inline bool AsyncFileNativeAio::PrepareWriteV(iovec* iov, int count, long long offset, AioCallback* callback)
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
inline AsyncResult<int, CoRtn> AsyncFileNativeAio::Commit()
{
    int ret = io_submit(this->m_ioctx, this->m_unfinished_io, this->m_iocb_ptrs.data());
    if( ret < 0 ) {
        this->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_LinuxAioSubmitError, -ret);
        return AsyncResult<int, CoRtn>(-1);
    }
    this->m_current_index = 0;
    dynamic_cast<details::FileIoWaitEvent*>(this->GetIOAction()->GetBindEvent())->ResetFileIoWaitEventType(details::kFileIoWaitEventTypeLinuxAio);
    return {this->GetIOAction(), nullptr};
}


inline bool AsyncFileNativeAio::IoFinished(uint64_t finished_io)
{
    uint32_t num = this->m_unfinished_io.load();
    if(!this->m_unfinished_io.compare_exchange_strong(num, num - finished_io)) return false;
    return this->m_unfinished_io == 0;
}


inline AsyncFileNativeAio::~AsyncFileNativeAio()
{
    io_destroy(this->m_ioctx);
    close(this->m_event_handle.fd);
}

#endif



inline AsyncTcpSocket::AsyncTcpSocket(details::EventEngine *engine)
    :m_io(std::make_shared<AsyncNetIo>(engine))
{
}


inline AsyncTcpSocket::AsyncTcpSocket(GHandle handle, details::EventEngine *engine)
    :m_io(std::make_shared<AsyncNetIo>(handle, engine))
{
}


inline bool AsyncTcpSocket::Socket() const
{
    return details::AsyncTcpSocket(m_io);
}


inline bool AsyncTcpSocket::Socket(GHandle handle)
{
    m_io->GetHandle() = handle;
    HandleOption option(handle);
    return option.HandleNonBlock();
}


inline bool AsyncTcpSocket::Bind(const std::string& addr, int port)
{
    return details::Bind(m_io, addr, port);
}


inline bool AsyncTcpSocket::Listen(int backlog)
{
    return details::Listen(m_io, backlog);
}

template <typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncTcpSocket::Connect(THost *addr)
{
    return details::AsyncConnect<CoRtn>(m_io, addr);
}

template <typename CoRtn>
inline AsyncResult<GHandle, CoRtn> AsyncTcpSocket::Accept(THost *addr)
{
    return details::AsyncAccept<CoRtn>(m_io, addr);
}

template <typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncTcpSocket::Recv(TcpIOVec *iov, size_t length)
{
    return details::AsyncRecv<CoRtn>(m_io, iov, length);
}

template <typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncTcpSocket::Send(TcpIOVec *iov, size_t length)
{
    return details::AsyncSend<CoRtn>(m_io, iov, length);
}

template <typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncTcpSocket::Close()
{
    return details::AsyncNetClose<CoRtn>(m_io);
}


inline AsyncTcpSslSocket::AsyncTcpSslSocket(details::EventEngine *engine)
    :m_io(std::make_shared<AsyncSslNetIo>(engine))
{
    if(!SslCtx) {
        SslCtx = GetGlobalSSLCtx();
    }
}


inline AsyncTcpSslSocket::AsyncTcpSslSocket(GHandle handle, details::EventEngine *engine)
    :m_io(std::make_shared<AsyncSslNetIo>(handle, engine))
{
    if(!SslCtx) {
        SslCtx = GetGlobalSSLCtx();
    }
}


inline SSL_CTX* AsyncTcpSslSocket::SslCtx = nullptr;


inline AsyncTcpSslSocket::AsyncTcpSslSocket(SSL *ssl, details::EventEngine *engine)
    :m_io(std::make_shared<AsyncSslNetIo>(ssl, engine))
{
    if(!SslCtx) {
        SslCtx = GetGlobalSSLCtx();
    }
}


inline bool AsyncTcpSslSocket::Socket() const
{
    if(!details::AsyncTcpSocket(m_io)){
        return false;
    }
    return details::AsyncSSLSocket(m_io, SslCtx);
}


inline bool AsyncTcpSslSocket::Socket(GHandle handle)
{
    m_io->GetHandle() = handle;
    HandleOption option(handle);
    if(!option.HandleNonBlock()) {
        return false;
    }
    return details::AsyncSSLSocket(m_io, SslCtx);;
}


inline bool AsyncTcpSslSocket::Bind(const std::string& addr, int port)
{
    return details::Bind(m_io, addr, port);
}


inline bool AsyncTcpSslSocket::Listen(int backlog)
{
    return details::Listen(m_io, backlog);
}

template <typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncTcpSslSocket::Connect(THost *addr)
{
    return details::AsyncConnect<CoRtn>(m_io, addr);
}

template <typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncTcpSslSocket::AsyncSSLConnect()
{
    return details::AsyncSSLConnect<CoRtn>(m_io);
}

template <typename CoRtn>
inline AsyncResult<GHandle, CoRtn> AsyncTcpSslSocket::Accept(THost *addr)
{
    return details::AsyncAccept<CoRtn>(m_io, addr);
}

template <typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncTcpSslSocket::SSLAccept()
{
    return details::AsyncSSLAccept<CoRtn>(m_io);
}

template <typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncTcpSslSocket::Recv(TcpIOVec *iov, size_t length)
{
    return details::AsyncSSLRecv<CoRtn>(m_io, iov, length);
}

template <typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncTcpSslSocket::Send(TcpIOVec *iov, size_t length)
{
    return details::AsyncSSLSend<CoRtn>(m_io, iov, length);
}

template <typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncTcpSslSocket::Close()
{
    return details::AsyncSSLClose<CoRtn>(m_io);
}


inline AsyncUdpSocket::AsyncUdpSocket(details::EventEngine *engine)
    :m_io(std::make_shared<AsyncNetIo>(engine))
{
}


inline AsyncUdpSocket::AsyncUdpSocket(GHandle handle, details::EventEngine *engine)
    :m_io(std::make_shared<AsyncNetIo>(handle, engine))
{
}


inline bool AsyncUdpSocket::Socket() const
{
    return details::AsyncUdpSocket(m_io);
}


inline bool AsyncUdpSocket::Bind(const std::string& addr, int port)
{
    return details::Bind(m_io, addr, port);
}

template <typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncUdpSocket::RecvFrom(UdpIOVec *iov, size_t length)
{
    return details::AsyncRecvFrom<CoRtn>(m_io, iov, length);
}

template <typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncUdpSocket::SendTo(UdpIOVec *iov, size_t length)
{
    return details::AsyncSendTo<CoRtn>(m_io, iov, length);
}

template <typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncUdpSocket::Close()
{
    return details::AsyncNetClose<CoRtn>(m_io);
}


inline AsyncFileDescriptor::AsyncFileDescriptor(details::EventEngine *engine)
    :m_io(std::make_shared<AsyncFileIo>(engine))
{
}


inline AsyncFileDescriptor::AsyncFileDescriptor(GHandle handle, details::EventEngine *engine)
    :m_io(std::make_shared<AsyncFileIo>(handle, engine))
{
}


inline bool AsyncFileDescriptor::Open(const char *path, int flags, mode_t mode)
{
    return details::AsyncFileOpen(m_io, path, flags, mode);
}

template <typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncFileDescriptor::Read(FileIOVec* iov, size_t length)
{
    return details::AsyncFileRead<CoRtn>(m_io, iov, length);
}

template <typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncFileDescriptor::Write(FileIOVec* iov, size_t length)
{
    return details::AsyncFileWrite<CoRtn>(m_io, iov, length);
}

template <typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncFileDescriptor::Close()
{
    return details::AsyncFileClose<CoRtn>(m_io);
}


#ifdef __linux__


inline AsyncFileNativeAioDescriptor::AsyncFileNativeAioDescriptor(details::EventEngine *engine, int maxevents)
    :m_io(std::make_shared<AsyncFileNativeAio>(engine, maxevents))
{
    m_io->InitialEventHandle();
}


inline AsyncFileNativeAioDescriptor::AsyncFileNativeAioDescriptor(GHandle handle, details::EventEngine *engine, int maxevents)
    :m_io(std::make_shared<AsyncFileNativeAio>(handle, engine, maxevents))
{
    m_io->InitialEventHandle();
}


inline bool AsyncFileNativeAioDescriptor::Open(const char *path, int flags, mode_t mode)
{
    return details::AsyncFileOpen(m_io, path, flags, mode);
}


inline bool AsyncFileNativeAioDescriptor::PrepareRead(char *buf, size_t len, long long offset, AioCallback *callback)
{
    return m_io->PrepareRead(buf, len, offset, callback);
}


inline bool AsyncFileNativeAioDescriptor::PrepareWrite(char *buf, size_t len, long long offset, AioCallback *callback)
{
    return m_io->PrepareWrite(buf, len, offset, callback);
}


inline bool AsyncFileNativeAioDescriptor::PrepareReadV(iovec *iov, int count, long long offset, AioCallback *callback)
{
    return m_io->PrepareReadV(iov, count, offset, callback);
}


inline bool AsyncFileNativeAioDescriptor::PrepareWriteV(iovec *iov, int count, long long offset, AioCallback *callback)
{
    return m_io->PrepareWriteV(iov, count, offset, callback);
}

template <typename CoRtn>
inline AsyncResult<int, CoRtn> AsyncFileNativeAioDescriptor::Commit()
{
    return m_io->Commit<CoRtn>();
}

template <typename CoRtn>
inline AsyncResult<bool, CoRtn> AsyncFileNativeAioDescriptor::Close()
{
    return details::AsyncFileClose<CoRtn>(m_io);
}


inline AsyncFileNativeAioDescriptor::~AsyncFileNativeAioDescriptor()
{
    m_io->CloseEventHandle();
}

#endif

}


#endif