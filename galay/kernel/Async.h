#ifndef __GALAY_ASYNC_H__
#define __GALAY_ASYNC_H__

#include "galay/common/Base.h"
#include "galay/common/Error.h"
#include "WaitAction.h"
#include <openssl/ssl.h>

#ifdef __linux__
    #include <libaio.h>
#endif

namespace galay::event
{
    class EventEngine;
}

namespace galay::action
{
    class NetEventAction;
    class FileIoEventAction;
}

namespace galay::async
{
class HandleOption
{
public:
    HandleOption(GHandle handle);
    bool HandleBlock();
    bool HandleNonBlock();
    bool HandleReuseAddr();
    bool HandleReusePort();
    uint32_t& GetErrorCode();
private:
    GHandle m_handle;
    uint32_t m_error_code;
};

struct NetAddr
{
    std::string m_ip;
    uint16_t m_port;
};

struct NetIOVec
{
    char* m_buf = nullptr;
    size_t m_len = 0;
};

struct FileIOVec
{
    GHandle m_handle;
    char* m_buf = nullptr;
    size_t m_length;
    long long m_offset;
};

class AsyncNetIo;
class AsyncSslNetIo;
class AsyncFileIo;

extern "C" 
{

/*
    ****************************
                net
    ****************************
*/

bool AsyncSocket(AsyncNetIo* asocket);
bool BindAndListen(AsyncNetIo* asocket, int port, int backlog);

coroutine::Awaiter_GHandle AsyncAccept(AsyncNetIo* asocket, NetAddr* addr);
coroutine::Awaiter_bool AsyncConnect(AsyncNetIo* asocket, NetAddr* addr);
coroutine::Awaiter_int AsyncRecv(AsyncNetIo* asocket, NetIOVec* iov);
coroutine::Awaiter_int AsyncSend(AsyncNetIo* asocket, NetIOVec* iov);
coroutine::Awaiter_bool AsyncClose(AsyncNetIo* asocket);

bool AsyncSSLSocket(AsyncSslNetIo* asocket, SSL_CTX* ctx);
/*
    must be called after AsyncAccept
*/
coroutine::Awaiter_bool AsyncSSLAccept(AsyncSslNetIo* asocket);
/*
    must be called after AsyncConnect
*/
coroutine::Awaiter_bool SSLConnect(AsyncSslNetIo* asocket);
coroutine::Awaiter_int AsyncSSLRecv(AsyncSslNetIo* asocket, NetIOVec *iov);
coroutine::Awaiter_int AsyncSSLSend(AsyncSslNetIo* asocket, NetIOVec *iov);
coroutine::Awaiter_bool AsyncSSLClose(AsyncSslNetIo* asocket);

/*
    ****************************
                file 
    ****************************
*/
coroutine::Awaiter_GHandle AsyncFileOpen(const char* path, int flags, mode_t mode);

}

class AsyncNetIo
{
public:
    using ptr = std::shared_ptr<AsyncNetIo>;
    AsyncNetIo(event::EventEngine* engine);

    HandleOption GetOption();
    inline GHandle& GetHandle() { return m_handle; }
    inline action::NetEventAction*& GetAction() { return m_action; };
    uint32_t &GetErrorCode();
    virtual ~AsyncNetIo();
protected:
    GHandle m_handle;
    uint32_t m_err_code;
    action::NetEventAction* m_action;
};

class AsyncTcpSocket: public AsyncNetIo
{
public:
    AsyncTcpSocket(event::EventEngine* engine);
};

class AsyncSslNetIo: public AsyncNetIo
{
public:
    AsyncSslNetIo(event::EventEngine* engine);
    inline SSL*& GetSSL() { return m_ssl; }
    virtual ~AsyncSslNetIo();
private:
    SSL* m_ssl = nullptr;
};

class AsyncSslTcpSocket: public AsyncSslNetIo
{
public:
    AsyncSslTcpSocket(event::EventEngine* engine);
};

class AsyncUdpSocket: public AsyncNetIo
{
public:
    using ptr = std::shared_ptr<AsyncUdpSocket>;
    AsyncUdpSocket(event::EventEngine* engine);
    ~AsyncUdpSocket();
private:
    GHandle m_handle;
    uint32_t m_error_code;
};

enum class FileIoType: int
{
    kFileIOLinuxNativeAio,
};

class AsyncFileIo
{
public:
    using ptr = std::shared_ptr<AsyncFileIo>;
    AsyncFileIo(event::EventEngine* engine);
    inline action::FileIoEventAction*& GetAction() { return m_action; };
    inline GHandle& GetHandle() { return m_handle; }
    inline uint32_t& GetErrorCode() { return m_error_code; }
    virtual ~AsyncFileIo();
private:
    // eventfd
    GHandle m_handle;
    uint32_t m_error_code;
    action::FileIoEventAction* m_action;
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
    AsyncFileNativeAio(int maxevents, event::EventEngine* engine);
    /*
        return false at m_current_index == maxevents or m_current_index atomic operation;
    */
    bool PrepareRead(GHandle handle, char* buf, size_t len, long long offset);
    bool PrepareWrite(GHandle handle, char* buf, size_t len, long long offset);

    bool PrepareReadV(GHandle handle, iovec* iov, int count, long long offset);
    bool PrepareWriteV(GHandle handle, iovec* iov, int count, long long offset);

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