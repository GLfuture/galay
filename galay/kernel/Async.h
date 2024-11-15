#ifndef __GALAY_ASYNC_H__
#define __GALAY_ASYNC_H__

#include "common/Base.h"
#include "common/Error.h"
#include "WaitAction.h"
#include <openssl/ssl.h>

namespace galay::event
{
    class EventEngine;
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


class AsyncTcpSocket
{
public:
    using ptr = std::shared_ptr<AsyncTcpSocket>;
    AsyncTcpSocket();
    AsyncTcpSocket(GHandle handle);
    HandleOption GetOption();

    static GHandle Socket();
    bool BindAndListen(int port, int backlog);
    

    coroutine::Awaiter_GHandle Accept(action::TcpEventAction* action);
    coroutine::Awaiter_bool Connect(action::TcpEventAction* action, NetAddr* addr);
    //return send length, -1 has error 0 disconnect 
    coroutine::Awaiter_int Recv(action::TcpEventAction* action);
    coroutine::Awaiter_int Send(action::TcpEventAction* action);
    coroutine::Awaiter_bool Close(action::TcpEventAction* action);
    //获取rbuffer，清空视图，注意内存释放(attention)
    inline std::string_view& GetRBuffer() { return m_rbuffer; }
    inline void SetRBuffer(std::string_view view) { m_rbuffer = view; }
    inline void SetWBuffer(std::string_view view) { m_wbuffer = view; }
    inline std::string_view& GetWBuffer() { return m_wbuffer; }
    inline GHandle& GetHandle() { return m_handle; }
    int GetRemotePort();
    std::string GetRemoteAddr();
    uint32_t &GetErrorCode();
    ~AsyncTcpSocket();
protected:
    GHandle m_handle;
    uint32_t m_err_code;
    std::string_view m_rbuffer;
    std::string_view m_wbuffer;
};

class AsyncTcpSslSocket: public AsyncTcpSocket
{
public:
    AsyncTcpSslSocket();
    AsyncTcpSslSocket(GHandle handle, SSL* ssl);
    static SSL* SSLSocket(GHandle& handle, SSL_CTX* ctx);
    coroutine::Awaiter_GHandle Accept(action::TcpSslEventAction* action);
    //
    coroutine::Awaiter_bool SSLAccept(action::TcpSslEventAction* action);
    coroutine::Awaiter_bool BindAndListen(int port, int backlog);
    coroutine::Awaiter_bool Connect(action::TcpSslEventAction* action, NetAddr* addr);
    //return send length, -1 has error 0 disconnect 
    coroutine::Awaiter_int SSLRecv(action::TcpSslEventAction* action);
    coroutine::Awaiter_int SSLSend(action::TcpSslEventAction* action);
    coroutine::Awaiter_bool SSLClose(action::TcpSslEventAction* action);
    coroutine::Awaiter_bool SSLConnect(action::TcpSslEventAction* action);
    inline SSL*& GetSSL() { return m_ssl; }
    ~AsyncTcpSslSocket();
protected:
    static GHandle Socket();
    coroutine::Awaiter_int Recv(action::TcpEventAction* action);
    coroutine::Awaiter_int Send(action::TcpEventAction* action);
    coroutine::Awaiter_bool Close(action::TcpEventAction* action);
private:
    SSL* m_ssl;
    AsyncTcpSocket m_socket;
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