#ifndef GALAY_ASYNC_HPP
#define GALAY_ASYNC_HPP


#include "Internal.hpp"
#include "Coroutine.hpp"
#include "ExternApi.hpp"
#include "WaitAction.h"
#include <openssl/ssl.h>

#ifdef __linux__
    #include <libaio.h>
#endif

#define DEFAULT_IO_EVENTS_SIZE      1024

namespace galay
{

class HandleOption
{
public:
    explicit HandleOption(GHandle handle);
    bool HandleBlock();
    bool HandleNonBlock();
    bool HandleReuseAddr();
    bool HandleReusePort();
    uint32_t& GetErrorCode();
private:
    GHandle m_handle;
    uint32_t m_error_code;
};

class AsyncNetIo
{
public:
    using ptr = std::shared_ptr<AsyncNetIo>;
    using wptr = std::weak_ptr<AsyncNetIo>;
    explicit AsyncNetIo(details::EventEngine* engine);
    explicit AsyncNetIo(GHandle handle, details::EventEngine* engine);

    HandleOption GetOption() const;
    GHandle& GetHandle() { return m_handle; }
    details::IOEventAction* GetAction() { return m_action; };
    uint32_t &GetErrorCode();
    virtual ~AsyncNetIo();
protected:
    virtual void ActionInit(details::EventEngine *engine);
protected:
    GHandle m_handle;
    uint32_t m_err_code;
    details::IOEventAction* m_action;
};


class AsyncSslNetIo: public AsyncNetIo
{
public:
    using ptr = std::shared_ptr<AsyncSslNetIo>;
    using wptr = std::weak_ptr<AsyncSslNetIo>;
    explicit AsyncSslNetIo(details::EventEngine* engine);
    explicit AsyncSslNetIo(GHandle handle, details::EventEngine* engine);
    explicit AsyncSslNetIo(SSL* ssl, details::EventEngine* engine);
    SSL*& GetSSL() { return m_ssl; }
    ~AsyncSslNetIo() override;
protected:
    void ActionInit(details::EventEngine *engine) override;
protected:
    SSL* m_ssl;
};


class AsyncFileIo
{
public:
    using ptr = std::shared_ptr<AsyncFileIo>;
    using wptr = std::weak_ptr<AsyncFileIo>;
    explicit AsyncFileIo(details::EventEngine* engine);
    explicit AsyncFileIo(GHandle handle, details::EventEngine* engine);
    [[nodiscard]] HandleOption GetOption() const;
    details::IOEventAction* GetAction() { return m_action; };
    GHandle& GetHandle() { return m_handle; }
    uint32_t& GetErrorCode() { return m_error_code; }
    virtual ~AsyncFileIo();
protected:
    // eventfd
    GHandle m_handle;
    uint32_t m_error_code;
    details::IOEventAction* m_action;
};

#ifdef  __linux__

class AioCallback 
{
public:
   virtual void OnAioComplete(io_event* event) = 0;
};


class AsyncFileNativeAio: public AsyncFileIo
{
public:
    using ptr = std::shared_ptr<AsyncFileNativeAio>;
    using wptr = std::weak_ptr<AsyncFileNativeAio>;
    explicit AsyncFileNativeAio(details::EventEngine* engine, int maxevents);
    explicit AsyncFileNativeAio(GHandle handle, details::EventEngine* engine, int maxevents);

    bool InitialEventHandle();
    bool InitialEventHandle(GHandle handle);
    bool CloseEventHandle();
    /*
        return false at m_current_index == maxevents or m_current_index atomic operation;
    */
    bool PrepareRead(char* buf, size_t len, long long offset, AioCallback* callback = nullptr);
    bool PrepareWrite(char* buf, size_t len, long long offset, AioCallback* callback = nullptr);

    bool PrepareReadV(iovec* iov, int count, long long offset, AioCallback* callback = nullptr);
    bool PrepareWriteV(iovec* iov, int count, long long offset, AioCallback* callback = nullptr);

    template<typename CoRtn = void>
    AsyncResult<int, CoRtn> Commit();
    
    GHandle GetEventHandle() { return m_event_handle; }
    io_context_t GetIoContext() { return m_ioctx; }
    bool IoFinished(uint64_t finished_io);
    uint32_t GetUnfinishedIO() { return m_current_index; }
    virtual ~AsyncFileNativeAio();
private:
    GHandle m_event_handle;
    io_context_t m_ioctx;
    std::vector<iocb> m_iocbs;
    std::vector<iocb*> m_iocb_ptrs; 
    std::atomic_uint32_t m_current_index;
    std::atomic_uint32_t m_unfinished_io;
};


#endif

class AsyncTcpSocket
{
public:
    explicit AsyncTcpSocket(details::EventEngine* engine);
    explicit AsyncTcpSocket(GHandle handle, details::EventEngine* engine);
    bool Socket() const;
    bool Socket(GHandle handle);
    bool Bind(const std::string& addr, int port);
    bool Listen(int backlog);
    template<typename CoRtn = void>
    AsyncResult<bool, CoRtn> Connect(NetAddr* addr);
    template<typename CoRtn = void>
    AsyncResult<GHandle, CoRtn> Accept(NetAddr* addr);
    template<typename CoRtn = void>
    AsyncResult<int, CoRtn> Recv(TcpIOVec* iov, size_t length);
    template<typename CoRtn = void>
    AsyncResult<int, CoRtn> Send(TcpIOVec* iov, size_t length);
    template<typename CoRtn = void>
    AsyncResult<bool, CoRtn> Close();

    GHandle GetHandle() const { return m_io->GetHandle(); }
    uint32_t GetErrorCode() const { return m_io->GetErrorCode(); }
    ~AsyncTcpSocket() = default;
private:
    AsyncNetIo::ptr m_io;
};


class AsyncTcpSslSocket
{
    static SSL_CTX* SslCtx;
public:
    explicit AsyncTcpSslSocket(details::EventEngine* engine);
    explicit AsyncTcpSslSocket(GHandle handle, details::EventEngine* engine);
    explicit AsyncTcpSslSocket(SSL* ssl, details::EventEngine* engine);
    bool Socket() const;
    bool Socket(GHandle handle);
    bool Bind(const std::string& addr, int port);
    bool Listen(int backlog);
    template<typename CoRtn = void>
    AsyncResult<bool, CoRtn> Connect(NetAddr* addr);
    template<typename CoRtn = void>
    AsyncResult<bool, CoRtn> AsyncSSLConnect();
    template<typename CoRtn = void>
    AsyncResult<GHandle, CoRtn> Accept(NetAddr* addr);
    template<typename CoRtn = void>
    AsyncResult<bool, CoRtn> SSLAccept();
    template<typename CoRtn = void>
    AsyncResult<int, CoRtn> Recv(TcpIOVec* iov, size_t length);
    template<typename CoRtn = void>
    AsyncResult<int, CoRtn> Send(TcpIOVec* iov, size_t length);
    template<typename CoRtn = void>
    AsyncResult<bool, CoRtn> Close();

    GHandle GetHandle() const { return m_io->GetHandle(); }
    uint32_t GetErrorCode() const { return m_io->GetErrorCode(); }
    ~AsyncTcpSslSocket() = default;
private:
    AsyncSslNetIo::ptr m_io;
};


class AsyncUdpSocket
{
public:
    using ptr = std::shared_ptr<AsyncUdpSocket>;
    explicit AsyncUdpSocket(details::EventEngine* engine);
    explicit AsyncUdpSocket(GHandle handle, details::EventEngine* engine);
    bool Socket() const;
    bool Bind(const std::string& addr, int port);
    template<typename CoRtn = void>
    AsyncResult<int, CoRtn> RecvFrom(UdpIOVec* iov, size_t length);
    template<typename CoRtn = void>
    AsyncResult<int, CoRtn> SendTo(UdpIOVec* iov, size_t length);
    template<typename CoRtn = void>
    AsyncResult<bool, CoRtn> Close();
    GHandle GetHandle() const { return m_io->GetHandle(); }
    uint32_t GetErrorCode() const { return m_io->GetErrorCode(); }
    ~AsyncUdpSocket() = default;
private:
    AsyncNetIo::ptr m_io;
};


class AsyncFileDescriptor
{
public:
    using ptr = std::shared_ptr<AsyncFileDescriptor>;
    explicit AsyncFileDescriptor(details::EventEngine* engine);
    explicit AsyncFileDescriptor(GHandle handle, details::EventEngine* engine);
    bool Open(const char* path, int flags, mode_t mode);
    template<typename CoRtn = void>
    AsyncResult<int, CoRtn> Read(FileIOVec* iov, size_t length);
    template<typename CoRtn = void>
    AsyncResult<int, CoRtn> Write(FileIOVec* iov, size_t length);
    template<typename CoRtn = void>
    AsyncResult<bool, CoRtn> Close();
    GHandle GetHandle() const { return m_io->GetHandle(); }
    uint32_t GetErrorCode() const { return m_io->GetErrorCode(); }
private:
    AsyncFileIo::ptr m_io;
};

#ifdef  __linux__

class AsyncFileNativeAioDescriptor
{
public:
    using ptr = std::shared_ptr<AsyncFileNativeAioDescriptor>;
    explicit AsyncFileNativeAioDescriptor(details::EventEngine* engine, int maxevents);
    explicit AsyncFileNativeAioDescriptor(GHandle handle, details::EventEngine* engine, int maxevents);
    bool Open(const char* path, int flags, mode_t mode);

    bool PrepareRead(char* buf, size_t len, long long offset, AioCallback* callback = nullptr);
    bool PrepareWrite(char* buf, size_t len, long long offset, AioCallback* callback = nullptr);

    bool PrepareReadV(iovec* iov, int count, long long offset, AioCallback* callback = nullptr);
    bool PrepareWriteV(iovec* iov, int count, long long offset, AioCallback* callback = nullptr);
    template<typename CoRtn = void>
    AsyncResult<int, CoRtn> Commit();
    template<typename CoRtn = void>
    AsyncResult<bool, CoRtn> Close();

    GHandle GetEventHandle() { return m_io->GetEventHandle(); }
    GHandle GetHandle() const { return m_io->GetHandle(); }
    uint32_t GetErrorCode() const { return m_io->GetErrorCode(); }
    ~AsyncFileNativeAioDescriptor();
private:
    AsyncFileNativeAio::ptr m_io;
};

#endif


}


namespace galay::details
{



enum NetWaitEventType
{
    kWaitEventTypeError,
    kTcpWaitEventTypeAccept,
    kTcpWaitEventTypeSslAccept,
    kTcpWaitEventTypeRecv,
    kTcpWaitEventTypeSslRecv,
    kTcpWaitEventTypeSend,
    kTcpWaitEventTypeSslSend,
    kTcpWaitEventTypeConnect,
    kTcpWaitEventTypeSslConnect,
    kWaitEventTypeClose,
    kWaitEventTypeSslClose,
    kUdpWaitEventTypeRecvFrom,
    kUdpWaitEventTypeSendTo,
};

enum CommonFailedType
{
    eCommonOtherFailed = -2,
    eCommonNonBlocking = -1,
    eCommonDisConnect = 0,
};

class NetWaitEvent: public WaitEvent
{
public:
    explicit NetWaitEvent(AsyncNetIo* socket);
    
    std::string Name() override;
    bool OnWaitPrepare(CoroutineBase::wptr co, void* ctx) override;
    void HandleEvent(EventEngine* engine) override;
    EventType GetEventType() override;
    GHandle GetHandle() override;
    void ResetNetWaitEventType(const NetWaitEventType type) { m_type = type; }
    AsyncNetIo* GetAsyncTcpSocket() const { return m_socket; }
    ~NetWaitEvent() override;
protected:
    bool OnTcpAcceptWaitPrepare(const CoroutineBase::wptr co, void* ctx);
    bool OnTcpRecvWaitPrepare(const CoroutineBase::wptr co, void* ctx);
    bool OnTcpSendWaitPrepare(const CoroutineBase::wptr co, void* ctx);
    bool OnTcpConnectWaitPrepare(const CoroutineBase::wptr co, void* ctx);
    bool OnCloseWaitPrepare(const CoroutineBase::wptr co, void* ctx);
    bool OnUdpRecvfromWaitPrepare(const CoroutineBase::wptr co, void* ctx);
    bool OnUdpSendtoWaitPrepare(const CoroutineBase::wptr co, void* ctx);


    void HandleErrorEvent(EventEngine* engine);
    void HandleTcpAcceptEvent(EventEngine* engine);
    void HandleTcpRecvEvent(EventEngine* engine);
    void HandleTcpSendEvent(EventEngine* engine);
    void HandleTcpConnectEvent(EventEngine* engine);
    void HandleCloseEvent(EventEngine* engine);
    void HandleUdpRecvfromEvent(EventEngine* engine);
    void HandleUdpSendtoEvent(EventEngine* engine);

    // return recvByte
    virtual int TcpDealRecv(TcpIOVec* iov);
    // return sendByte
    virtual int TcpDealSend(TcpIOVec* iov);

    virtual int UdpDealRecvfrom(UdpIOVec *iov);
    virtual int UdpDealSendto(UdpIOVec* iov);
protected:
    NetWaitEventType m_type;
    void *m_ctx{};
    AsyncNetIo* m_socket;
};

class NetSslWaitEvent final : public NetWaitEvent
{
public:
    explicit NetSslWaitEvent(AsyncSslNetIo* socket);
    std::string Name() override;
    EventType GetEventType() override;
    bool OnWaitPrepare(CoroutineBase::wptr co, void* ctx) override;
    void HandleEvent(EventEngine* engine) override;
    AsyncSslNetIo* GetAsyncTcpSocket() const;
    ~NetSslWaitEvent() override = default;
protected:
    bool OnTcpSslAcceptWaitPrepare(const CoroutineBase::wptr co, void* ctx);
    bool OnTcpSslConnectWaitPrepare(const CoroutineBase::wptr co, void* ctx);
    bool OnTcpSslRecvWaitPrepare(const CoroutineBase::wptr co, void* ctx);
    bool OnTcpSslSendWaitPrepare(const CoroutineBase::wptr co, void* ctx);
    bool OnTcpSslCloseWaitPrepare(const CoroutineBase::wptr co, void* ctx);

    void HandleTcpSslAcceptEvent(EventEngine* engine);
    void HandleTcpSslConnectEvent(EventEngine* engine);
    void HandleTcpSslRecvEvent(EventEngine* engine);
    void HandleTcpSslSendEvent(EventEngine* engine);
    void HandleTcpSslCloseEvent(EventEngine* engine);

    EventType CovertSSLErrorToEventType() const;
    // return recvByte
    int TcpDealRecv(TcpIOVec* iovc) override;
    // return sendByte
    int TcpDealSend(TcpIOVec* iovc) override;
private:
    int m_ssl_error{};
};


class UdpWaitEvent: public WaitEvent
{
public:
    
private:
    sockaddr* m_ToAddr = nullptr;
    sockaddr* m_FromAddr = nullptr;
};

enum FileIoWaitEventType
{
    kFileIoWaitEventTypeError,
#ifdef __linux__
    kFileIoWaitEventTypeLinuxAio,
#endif
    kFileIoWaitEventTypeRead,
    kFileIoWaitEventTypeWrite,
    kFileIoWaitEventTypeClose
};


class FileIoWaitEvent final : public WaitEvent
{
public:
    explicit FileIoWaitEvent(AsyncFileIo* fileio);
    std::string Name() override;
    bool OnWaitPrepare(CoroutineBase::wptr co, void* ctx) override;
    void HandleEvent(EventEngine* engine) override;
    EventType GetEventType() override;
    GHandle GetHandle() override;
    void ResetFileIoWaitEventType(FileIoWaitEventType type) { m_type = type; }
    AsyncFileIo* GetAsyncTcpSocket() const { return m_fileio; }
    ~FileIoWaitEvent() override;
private:
#ifdef __linux__
    bool OnAioWaitPrepare(CoroutineBase::wptr co, void* ctx);
    void HandleAioEvent(EventEngine* engine);
#endif
    bool OnKReadWaitPrepare(CoroutineBase::wptr co, void* ctx);
    void HandleKReadEvent(EventEngine* engine);
    bool OnKWriteWaitPrepare(CoroutineBase::wptr co, void* ctx);
    void HandleKWriteEvent(EventEngine* engine);

    bool OnCloseWaitPrepare(const CoroutineBase::wptr co, void* ctx);
    
private:
    void *m_ctx{};
    AsyncFileIo* m_fileio;
    FileIoWaitEventType m_type;
};

}


namespace galay::details 
{

/*
****************************
            net
****************************
*/

bool AsyncTcpSocket(AsyncNetIo::wptr asocket);

bool AsyncUdpSocket(AsyncNetIo::wptr asocket);

bool Bind(AsyncNetIo::wptr asocket, const std::string& addr, int port);

bool Listen(AsyncNetIo::wptr asocket, int backlog);
template<typename CoRtn = void>
AsyncResult<GHandle, CoRtn> AsyncAccept(AsyncNetIo::wptr asocket, NetAddr* addr);
template<typename CoRtn = void>
AsyncResult<bool, CoRtn> AsyncConnect(AsyncNetIo::wptr async_socket, NetAddr* addr);
/*
    return: 
        >0   bytes read
        0   close connection
        <0  error
*/
template<typename CoRtn = void>
AsyncResult<int, CoRtn> AsyncRecv(AsyncNetIo::wptr asocket, TcpIOVec* iov, size_t length);
/*
    return: 
        >0   bytes send
        <0  error
*/
template<typename CoRtn = void>
AsyncResult<int, CoRtn> AsyncSend(AsyncNetIo::wptr asocket, TcpIOVec* iov, size_t length);
/*

*/
template<typename CoRtn = void>
AsyncResult<int, CoRtn> AsyncRecvFrom(AsyncNetIo::wptr asocket, UdpIOVec* iov, size_t length);
template<typename CoRtn = void>
AsyncResult<int, CoRtn> AsyncSendTo(AsyncNetIo::wptr asocket, UdpIOVec* iov, size_t length);
template<typename CoRtn = void>
AsyncResult<bool, CoRtn> AsyncNetClose(AsyncNetIo::wptr asocket);

bool AsyncSSLSocket(AsyncSslNetIo::wptr asocket, SSL_CTX* ctx);
/*
    must be called after AsyncAccept
*/
template<typename CoRtn = void>
AsyncResult<bool, CoRtn> AsyncSSLAccept(AsyncSslNetIo::wptr asocket);
/*
    must be called after AsyncConnect
*/
template<typename CoRtn = void>
AsyncResult<bool, CoRtn> AsyncSSLConnect(AsyncSslNetIo::wptr asocket);
template<typename CoRtn = void>
AsyncResult<int, CoRtn> AsyncSSLRecv(AsyncSslNetIo::wptr asocket, TcpIOVec *iov, size_t length);
template<typename CoRtn = void>
AsyncResult<int, CoRtn> AsyncSSLSend(AsyncSslNetIo::wptr asocket, TcpIOVec *iov, size_t length);
template<typename CoRtn = void>
AsyncResult<bool, CoRtn> AsyncSSLClose(AsyncSslNetIo::wptr asocket);

/*
    ****************************
                file 
    ****************************
*/

bool AsyncFileOpen(AsyncFileIo::wptr afileio, const char* path, int flags, mode_t mode);
template<typename CoRtn = void>
AsyncResult<int, CoRtn> AsyncFileRead(AsyncFileIo::wptr afile, FileIOVec* iov, size_t length);
template<typename CoRtn = void>
extern AsyncResult<int, CoRtn> AsyncFileWrite(AsyncFileIo::wptr afile, FileIOVec* iov, size_t length);
template<typename CoRtn = void>
extern AsyncResult<bool, CoRtn> AsyncFileClose(AsyncFileIo::wptr afile);



}

#include "Async.tcc"

#endif