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

namespace galay::details
{

const int TIMEOUT_RETURN = -2;

enum ResumeInterfaceType
{
    kResumeInterfaceType_Accept = 0,
    kResumeInterfaceType_Connect,
    kResumeInterfaceType_Recv,
    kResumeInterfaceType_Send,
    kResumeInterfaceType_RecvFrom,
    kResumeInterfaceType_SendTo,
    kResumeInterfaceType_SSLAccept,
    kResumeInterfaceType_SSLConnect,
    kResumeInterfaceType_SSLRead,
    kResumeInterfaceType_SSLWrite,
    kResumeInterfaceType_FileRead,
    kResumeInterfaceType_FileWrite
};

class Resumer: public std::enable_shared_from_this<Resumer>
{
public:
    using ptr = std::shared_ptr<Resumer>;
    using time_event_wptr = std::weak_ptr<details::TimeEvent>;
    Resumer(int64_t& timeout);
    
    void StartTimeout(int64_t timeout);
    void Reset(CoroutineBase::wptr waitco);
    void Reset(CoroutineBase::wptr waitco, Timer::ptr timer);
    bool IsResumed();
    bool Resume();

    void SetResumeInterfaceType(ResumeInterfaceType type);
    CoroutineBase::wptr GetWaitCo();
    int64_t GetLastestEndTime();

private:
    static void DoingAsyncTimeout(Resumer::ptr resumer, time_event_wptr event, Timer::ptr timer);
private:
    int64_t& m_timeout;
    ResumeInterfaceType m_type;
    int64_t m_lastest_end_time = 0;
    Timer::ptr m_timer = nullptr;
    std::atomic_bool m_resumed = false;
    CoroutineBase::wptr m_waitco;
};

}

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


// one coroutine one context

/*
    一个接口用同一个context
*/
class AsyncNetEventContext
{
public:

    static AsyncNetEventContext* Create();
    static AsyncNetEventContext* Create(GHandle handle);

    void Resume();

    virtual ~AsyncNetEventContext();

    GHandle m_handle = {};
    uint32_t m_error_code = 0;
    int64_t m_timeout = 0;
    details::Resumer::ptr m_resumer = nullptr;
    details::IOEventAction* m_action = nullptr;
};


class AsyncSslNetEventContext: public AsyncNetEventContext
{
public:
    static AsyncSslNetEventContext* Create();
    static AsyncSslNetEventContext* Create(GHandle handle);
    static AsyncSslNetEventContext* Create(SSL* ssl);

    virtual ~AsyncSslNetEventContext();

    SSL* m_ssl;
};


class AsyncFileEventContext
{
public:

    static AsyncFileEventContext* Create();
    static AsyncFileEventContext* Create(GHandle handle);
    
    virtual ~AsyncFileEventContext();

    void Resume();

    GHandle m_handle = {};
    int64_t m_timeout = 0;
    uint32_t m_error_code = 0;
    details::Resumer::ptr m_resumer = nullptr;
    details::IOEventAction* m_action = nullptr;
};

#ifdef  __linux__

class AioCallback 
{
public:
   virtual void OnAioComplete(io_event* event) = 0;
};


class AsyncLinuxFileEventContext: public AsyncFileEventContext
{
public:

    static AsyncLinuxFileEventContext* Create(int maxevents);
    static AsyncLinuxFileEventContext* Create(GHandle handle, int maxevents);

    explicit AsyncLinuxFileEventContext(int maxevents);
    
    bool IoFinished(uint64_t finished_io);
    virtual ~AsyncLinuxFileEventContext();

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
    explicit AsyncTcpSocket();
    explicit AsyncTcpSocket(GHandle handle);
    bool Socket(bool noblock = true) const;
    bool Socket(GHandle handle);
    bool Bind(const std::string& addr, int port);
    bool Listen(int backlog);
    template<typename CoRtn = void>
    AsyncResult<bool, CoRtn> Connect(THost* addr, int64_t timeout = -1);
    template<typename CoRtn = void>
    AsyncResult<GHandle, CoRtn> Accept(THost* addr, int64_t timeout = -1);
    template<typename CoRtn = void>
    AsyncResult<int, CoRtn> Recv(TcpIOVec* iov, size_t length, int64_t timeout = -1);
    template<typename CoRtn = void>
    AsyncResult<int, CoRtn> Send(TcpIOVec* iov, size_t length, int64_t timeout = -1);
    template<typename CoRtn = void>
    AsyncResult<bool, CoRtn> Close();

    GHandle GetHandle() const;
    uint32_t GetErrorCode() const;
    virtual ~AsyncTcpSocket();
private:
    details::IOEventAction::ptr m_action;
    AsyncNetEventContext* m_async_context;
};


class AsyncTcpSslSocket
{
    static SSL_CTX* SslCtx;
public:
    explicit AsyncTcpSslSocket();
    explicit AsyncTcpSslSocket(GHandle handle);
    explicit AsyncTcpSslSocket(SSL* ssl);
    bool Socket(bool noblock = true) const;
    bool Socket(GHandle handle);
    bool Bind(const std::string& addr, int port);
    bool Listen(int backlog);
    template<typename CoRtn = void>
    AsyncResult<bool, CoRtn> Connect(THost* addr, int64_t timeout = -1);
    template<typename CoRtn = void>
    AsyncResult<bool, CoRtn> AsyncSSLConnect(int64_t timeout = -1);
    template<typename CoRtn = void>
    AsyncResult<GHandle, CoRtn> Accept(THost* addr, int64_t timeout = -1);
    template<typename CoRtn = void>
    AsyncResult<bool, CoRtn> SSLAccept(int64_t timeout = -1);
    template<typename CoRtn = void>
    AsyncResult<int, CoRtn> Recv(TcpIOVec* iov, size_t length, int64_t timeout = -1);
    template<typename CoRtn = void>
    AsyncResult<int, CoRtn> Send(TcpIOVec* iov, size_t length, int64_t timeout = -1);
    template<typename CoRtn = void>
    AsyncResult<bool, CoRtn> Close();

    GHandle GetHandle() const;
    uint32_t GetErrorCode() const;
    ~AsyncTcpSslSocket() = default;
private:
    AsyncSslNetEventContext* m_async_context;
};


class AsyncUdpSocket
{
public:
    using ptr = std::shared_ptr<AsyncUdpSocket>;
    explicit AsyncUdpSocket();
    explicit AsyncUdpSocket(GHandle handle);
    bool Socket(bool noblock = true) const;
    bool Bind(const std::string& addr, int port);
    template<typename CoRtn = void>
    AsyncResult<int, CoRtn> RecvFrom(UdpIOVec* iov, size_t length, int64_t timeout = -1);
    template<typename CoRtn = void>
    AsyncResult<int, CoRtn> SendTo(UdpIOVec* iov, size_t length, int64_t timeout = -1);
    template<typename CoRtn = void>
    AsyncResult<bool, CoRtn> Close();
    GHandle GetHandle() const;
    uint32_t GetErrorCode() const;
    virtual ~AsyncUdpSocket();
private:
    AsyncNetEventContext* m_async_context;
};


class AsyncFileDescriptor
{
public:
    using ptr = std::shared_ptr<AsyncFileDescriptor>;
    explicit AsyncFileDescriptor();
    explicit AsyncFileDescriptor(GHandle handle);
    bool Open(const char* path, int flags, mode_t mode);
    template<typename CoRtn = void>
    AsyncResult<int, CoRtn> Read(FileIOVec* iov, size_t length, int64_t timeout = -1);
    template<typename CoRtn = void>
    AsyncResult<int, CoRtn> Write(FileIOVec* iov, size_t length, int64_t timeout = -1);
    template<typename CoRtn = void>
    AsyncResult<bool, CoRtn> Close();
    GHandle GetHandle() const;
    uint32_t GetErrorCode() const;
    virtual ~AsyncFileDescriptor();
private:
    AsyncFileEventContext* m_async_context;
};

#ifdef  __linux__

class AsyncFileNativeAioDescriptor
{
public:
    using ptr = std::shared_ptr<AsyncFileNativeAioDescriptor>;
    explicit AsyncFileNativeAioDescriptor(int maxevents);
    explicit AsyncFileNativeAioDescriptor(GHandle handle, int maxevents);
    bool Open(const char* path, int flags, mode_t mode);

    bool PrepareRead(char* buf, size_t len, long long offset, AioCallback* callback = nullptr);
    bool PrepareWrite(char* buf, size_t len, long long offset, AioCallback* callback = nullptr);

    bool PrepareReadV(iovec* iov, int count, long long offset, AioCallback* callback = nullptr);
    bool PrepareWriteV(iovec* iov, int count, long long offset, AioCallback* callback = nullptr);
    template<typename CoRtn = void>
    AsyncResult<int, CoRtn> Commit();
    template<typename CoRtn = void>
    AsyncResult<bool, CoRtn> Close();

    GHandle GetEventHandle() { return m_async_context->m_event_handle; }
    GHandle GetHandle() const { return m_async_context->m_handle; }
    uint32_t GetErrorCode() const { return m_async_context->m_error_code; }
    ~AsyncFileNativeAioDescriptor();
private:
    AsyncLinuxFileEventContext* m_async_context;
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
    eCommonOtherFailed = -3,
    eCommonTimeOutFailed = -2,
    eCommonNonBlocking,
    eCommonDisConnect = 0,
};

class NetWaitEvent: public WaitEvent
{
public:
    explicit NetWaitEvent(AsyncNetEventContext* context);
    
    std::string Name() override;
    bool OnWaitPrepare(CoroutineBase::wptr co, void* ctx) override;
    void HandleEvent(EventEngine* engine) override;
    EventType GetEventType() override;
    GHandle GetHandle() override;
    void ResetNetWaitEventType(const NetWaitEventType type) { m_type = type; }
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
    void *m_ctx = nullptr;
    AsyncNetEventContext* m_async_context;
};

class NetSslWaitEvent final : public NetWaitEvent
{
public:
    explicit NetSslWaitEvent(AsyncSslNetEventContext* async_context);
    std::string Name() override;
    EventType GetEventType() override;
    bool OnWaitPrepare(CoroutineBase::wptr co, void* ctx) override;
    void HandleEvent(EventEngine* engine) override;
    AsyncSslNetEventContext* GetAsyncContext() const;
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
    int m_ssl_error = 0;
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
    explicit FileIoWaitEvent(AsyncFileEventContext* async_context);
    std::string Name() override;
    bool OnWaitPrepare(CoroutineBase::wptr co, void* ctx) override;
    void HandleEvent(EventEngine* engine) override;
    EventType GetEventType() override;
    GHandle GetHandle() override;
    void ResetFileIoWaitEventType(FileIoWaitEventType type) { m_type = type; }
    AsyncFileEventContext* GetAsyncContext() const { return m_async_context; }
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
    void *m_ctx = nullptr;
    AsyncFileEventContext* m_async_context;
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

bool AsyncTcpSocket(AsyncNetEventContext* async_context, bool noblock);

bool AsyncUdpSocket(AsyncNetEventContext* async_context, bool noblock);

bool Bind(AsyncNetEventContext* async_context, const std::string& addr, int port);

bool Listen(AsyncNetEventContext* async_context, int backlog);
template<typename CoRtn = void>
AsyncResult<GHandle, CoRtn> AsyncAccept(AsyncNetEventContext* async_context, THost* addr, int64_t timeout);
template<typename CoRtn = void>
AsyncResult<bool, CoRtn> AsyncConnect(AsyncNetEventContext* async_context, THost* addr, int64_t timeout);
/*
    return: 
        >0   bytes read
        0   close connection
        <0  error
*/
template<typename CoRtn = void>
AsyncResult<int, CoRtn> AsyncRecv(AsyncNetEventContext* async_context, TcpIOVec* iov, size_t length, int64_t timeout);
/*
    return: 
        >0   bytes send
        <0  error
*/
template<typename CoRtn = void>
AsyncResult<int, CoRtn> AsyncSend(AsyncNetEventContext* async_context, TcpIOVec* iov, size_t length, int64_t timeout);
/*

*/
template<typename CoRtn = void>
AsyncResult<int, CoRtn> AsyncRecvFrom(AsyncNetEventContext* async_context, UdpIOVec* iov, size_t length, int64_t timeout);
template<typename CoRtn = void>
AsyncResult<int, CoRtn> AsyncSendTo(AsyncNetEventContext* async_context, UdpIOVec* iov, size_t length, int64_t timeout);
template<typename CoRtn = void>
AsyncResult<bool, CoRtn> AsyncNetClose(AsyncNetEventContext* async_context);

bool AsyncSSLSocket(AsyncSslNetEventContext* async_context, SSL_CTX* ctx);
/*
    must be called after AsyncAccept
*/
template<typename CoRtn = void>
AsyncResult<bool, CoRtn> AsyncSSLAccept(AsyncSslNetEventContext* async_context, int64_t timeout);
/*
    must be called after AsyncConnect
*/
template<typename CoRtn = void>
AsyncResult<bool, CoRtn> AsyncSSLConnect(AsyncSslNetEventContext* async_context, int64_t timeout);
template<typename CoRtn = void>
AsyncResult<int, CoRtn> AsyncSSLRecv(AsyncSslNetEventContext* async_context, TcpIOVec *iov, size_t length, int64_t timeout);
template<typename CoRtn = void>
AsyncResult<int, CoRtn> AsyncSSLSend(AsyncSslNetEventContext* async_context, TcpIOVec *iov, size_t length, int64_t timeout);
template<typename CoRtn = void>
AsyncResult<bool, CoRtn> AsyncSSLClose(AsyncSslNetEventContext* async_context);

/*
    ****************************
                file 
    ****************************
*/

bool AsyncFileOpen(AsyncFileEventContext* afileio, const char* path, int flags, mode_t mode);
template<typename CoRtn = void>
AsyncResult<int, CoRtn> AsyncFileRead(AsyncFileEventContext* afile, FileIOVec* iov, size_t length, int64_t timeout);
template<typename CoRtn = void>
extern AsyncResult<int, CoRtn> AsyncFileWrite(AsyncFileEventContext* afile, FileIOVec* iov, size_t length, int64_t timeout);
template<typename CoRtn = void>
extern AsyncResult<bool, CoRtn> AsyncFileClose(AsyncFileEventContext* afile);




}

#include "Async.tcc"

#endif