#ifndef GALAY_ASYNC_H
#define GALAY_ASYNC_H

#include "galay/common/Base.h"
#include "galay/common/Error.h"
#include "WaitAction.h"
#include <openssl/ssl.h>

#ifdef __linux__
    #include <libaio.h>
#endif

namespace galay::details
{
    class EventEngine;
}

namespace galay
{

struct NetAddr;
struct TcpIOVec;
struct UdpIOVec;
struct FileIOVec;

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

    Awaiter<int> Commit();
    
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
    Awaiter<bool> Connect(NetAddr* addr);
    Awaiter<GHandle> Accept(NetAddr* addr);
    Awaiter<int> Recv(TcpIOVec* iov, size_t length);
    Awaiter<int> Send(TcpIOVec* iov, size_t length);
    Awaiter<bool> Close();

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
    Awaiter<bool> Connect(NetAddr* addr);
    Awaiter<bool> AsyncSSLConnect();
    Awaiter<GHandle> Accept(NetAddr* addr);
    Awaiter<bool> SSLAccept();
    Awaiter<int> Recv(TcpIOVec* iov, size_t length);

    Awaiter<int> Send(TcpIOVec* iov, size_t length);
    Awaiter<bool> Close();

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
    Awaiter<int> RecvFrom(UdpIOVec* iov, size_t length);
    Awaiter<int> SendTo(UdpIOVec* iov, size_t length);
    Awaiter<bool> Close();
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
    Awaiter<int> Read(FileIOVec* iov, size_t length);
    Awaiter<int> Write(FileIOVec* iov, size_t length);
    Awaiter<bool> Close();
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

    Awaiter<int> Commit();
    Awaiter<bool> Close();

    GHandle GetEventHandle() { return m_io->GetEventHandle(); }
    GHandle GetHandle() const { return m_io->GetHandle(); }
    uint32_t GetErrorCode() const { return m_io->GetErrorCode(); }
    ~AsyncFileNativeAioDescriptor();
private:
    AsyncFileNativeAio::ptr m_io;
};

#endif


}
#endif