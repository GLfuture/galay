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

namespace galay::async
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
    explicit AsyncNetIo(details::EventEngine* engine);

    HandleOption GetOption() const;
    GHandle& GetHandle() { return m_handle; }
    details::IOEventAction* GetAction() { return m_action; };
    uint32_t &GetErrorCode();
    virtual ~AsyncNetIo();
protected:
    GHandle m_handle;
    uint32_t m_err_code;
    details::IOEventAction* m_action;
};

class AsyncFileIo
{
public:
    using ptr = std::shared_ptr<AsyncFileIo>;
    explicit AsyncFileIo(details::EventEngine* engine);
    [[nodiscard]] HandleOption GetOption() const;
    details::IOEventAction* GetAction() { return m_action; };
    GHandle& GetHandle() { return m_handle; }
    uint32_t& GetErrorCode() { return m_error_code; }
    virtual ~AsyncFileIo();
private:
    // eventfd
    GHandle m_handle;
    uint32_t m_error_code;
    details::IOEventAction* m_action;
};


class AsyncSslNetIo: public AsyncNetIo
{
public:
    explicit AsyncSslNetIo(details::EventEngine* engine);
    SSL*& GetSSL() { return m_ssl; }
    ~AsyncSslNetIo() override;
private:
    SSL* m_ssl = nullptr;
};

class AsyncTcpSocket: public AsyncNetIo
{
public:
    explicit AsyncTcpSocket(details::EventEngine* engine);
};

class AsyncSslTcpSocket: public AsyncSslNetIo
{
public:
    explicit AsyncSslTcpSocket(details::EventEngine* engine);
};

class AsyncUdpSocket: public AsyncNetIo
{
public:
    using ptr = std::shared_ptr<AsyncUdpSocket>;
    explicit AsyncUdpSocket(details::EventEngine* engine);
    ~AsyncUdpSocket() override;
private:
    GHandle m_handle;
    uint32_t m_error_code;
};


class AsyncFileDescriptor final : public AsyncFileIo
{
public:
    using ptr = std::shared_ptr<AsyncFileDescriptor>;
    explicit AsyncFileDescriptor(details::EventEngine* engine);
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
    AsyncFileNativeAio(int maxevents, details::EventEngine* engine);
    /*
        return false at m_current_index == maxevents or m_current_index atomic operation;
    */
    bool PrepareRead(GHandle handle, char* buf, size_t len, long long offset, AioCallback* callback = nullptr);
    bool PrepareWrite(GHandle handle, char* buf, size_t len, long long offset, AioCallback* callback = nullptr);

    bool PrepareReadV(GHandle handle, iovec* iov, int count, long long offset, AioCallback* callback = nullptr);
    bool PrepareWriteV(GHandle handle, iovec* iov, int count, long long offset, AioCallback* callback = nullptr);

    coroutine::Awaiter_int Commit();
    
    inline io_context_t GetIoContext() { return m_ioctx; }
    bool IoFinished(uint64_t finished_io);
    uint32_t GetUnfinishedIO() { return m_current_index; }

    virtual ~AsyncFileNativeAio();
private:
    io_context_t m_ioctx;
    std::vector<iocb> m_iocbs;
    std::vector<iocb*> m_iocb_ptrs; 
    std::atomic_uint32_t m_current_index;
    std::atomic_uint32_t m_unfinished_io;
};


#endif
}
#endif