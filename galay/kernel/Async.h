#ifndef __GALAY_ASYNC_H__
#define __GALAY_ASYNC_H__

#include "../common/Base.h"
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
    std::string GetLastError();
private:
    GHandle m_handle;
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
    coroutine::Awaiter_bool InitialHandle(action::NetIoEventAction* action);
    coroutine::Awaiter_GHandle Accept(action::NetIoEventAction* action);
    coroutine::Awaiter_bool BindAndListen(int port, int backlog);
    coroutine::Awaiter_bool Connect(action::NetIoEventAction* action, const NetAddr& addr);
    //return send length, -1 has error 0 disconnect 
    coroutine::Awaiter_int Recv(action::NetIoEventAction* action);
    coroutine::Awaiter_int Send(action::NetIoEventAction* action);
    coroutine::Awaiter_bool Close(action::NetIoEventAction* action);
    
    //获取rbuffer，清空视图，注意内存释放(attention)
    inline std::string_view& GetRBuffer() { return m_rbuffer; }
    inline void SetRBuffer(std::string_view view) { m_rbuffer = view; }
    inline void SetWBuffer(std::string_view view) { m_wbuffer = view; }
    inline std::string_view& GetWBuffer() { return m_wbuffer; }
    inline NetAddr GetConnectAddr() { return m_connect_addr; }

    inline GHandle& GetHandle() { return m_handle; }
    
    int GetRemotePort();
    std::string GetRemoteAddr();
    
    void SetLastError(std::string error);
    std::string GetLastError();
    ~AsyncTcpSocket();
private:
    GHandle m_handle;
    NetAddr m_connect_addr;
    std::string m_last_error;
    std::string_view m_rbuffer;
    std::string_view m_wbuffer;
};

class AsyncTcpSslSocket
{
public:
    coroutine::Awaiter_bool Connect(action::NetIoEventAction* action, const NetAddr& addr);
    coroutine::Awaiter_bool SSLConnect(action::NetIoEventAction* action);
    coroutine::Awaiter_GHandle Accept(action::NetIoEventAction *action);
    coroutine::Awaiter_bool SSLAccept();

    inline SSL* GetSSL() { return m_ssl; }
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
    coroutine::Awaiter_bool InitialHandle();
    
    coroutine::Awaiter_bool Close();
    ~AsyncUdpSocket();
private:
    GHandle m_handle;
    bool m_handle_closed;
    std::string m_last_error;
};

}
#endif