#ifndef __GALAY_ASYNC_H__
#define __GALAY_ASYNC_H__

#include "galay/common/Base.h"
#include "galay/common/Error.h"
#include "WaitAction.h"
#include <openssl/ssl.h>

namespace galay::event
{
    class EventEngine;
}

namespace galay::action
{
    class TcpEventAction;
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

struct IOVec
{
    char* m_buf = nullptr;
    size_t m_len = 0;
};

class AsyncTcpSocket;
class AsyncTcpSslSocket;

extern "C" 
{
bool AsyncSocket(AsyncTcpSocket* asocket);
bool BindAndListen(AsyncTcpSocket* asocket, int port, int backlog);
coroutine::Awaiter_GHandle AsyncAccept(AsyncTcpSocket* asocket, NetAddr* addr);
coroutine::Awaiter_bool AsyncConnect(AsyncTcpSocket* asocket, NetAddr* addr);
coroutine::Awaiter_int AsyncRecv(AsyncTcpSocket* asocket, IOVec* iov);
coroutine::Awaiter_int AsyncSend(AsyncTcpSocket* asocket, IOVec* iov);
coroutine::Awaiter_bool AsyncClose(AsyncTcpSocket* asocket);

bool AsyncSSLSocket(AsyncTcpSslSocket* asocket, SSL_CTX* ctx);
/*
    must be called after AsyncAccept
*/
coroutine::Awaiter_bool AsyncSSLAccept(AsyncTcpSslSocket* asocket);
/*
    must be called after AsyncConnect
*/
coroutine::Awaiter_bool SSLConnect(AsyncTcpSslSocket* asocket);
coroutine::Awaiter_int AsyncSSLRecv(AsyncTcpSslSocket* asocket, IOVec *iov);
coroutine::Awaiter_int AsyncSSLSend(AsyncTcpSslSocket* asocket, IOVec *iov);
coroutine::Awaiter_bool AsyncSSLClose(AsyncTcpSslSocket* asocket);


}

class AsyncTcpSocket
{
public:
    using ptr = std::shared_ptr<AsyncTcpSocket>;
    AsyncTcpSocket(event::EventEngine* engine);

    HandleOption GetOption();
    inline GHandle& GetHandle() { return m_handle; }
    inline action::TcpEventAction*& GetAction() { return m_action; };
    uint32_t &GetErrorCode();
    virtual ~AsyncTcpSocket();
protected:
    GHandle m_handle;
    uint32_t m_err_code;
    action::TcpEventAction* m_action;
};

class AsyncTcpSslSocket: public AsyncTcpSocket
{
public:
    AsyncTcpSslSocket(event::EventEngine* engine);
    inline SSL*& GetSSL() { return m_ssl; }
    virtual ~AsyncTcpSslSocket();
private:
    SSL* m_ssl = nullptr;
};

class AsyncUdpSocket
{
public:
    using ptr = std::shared_ptr<AsyncUdpSocket>;
    AsyncUdpSocket();
    HandleOption GetOption();
    coroutine::Awaiter_bool Socket();
    //coroutine::Awaiter_int RecvFrom(action::U* action);
    coroutine::Awaiter_bool Close();
    ~AsyncUdpSocket();
private:
    GHandle m_handle;
    uint32_t m_error_code;
};

}
#endif