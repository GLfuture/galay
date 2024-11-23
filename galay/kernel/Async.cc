#include "Async.h"
#include "Event.h"
#include "EventEngine.h"
#include "WaitAction.h"
#if defined(__linux__)
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <fcntl.h>
#elif defined(WIN32) || defined(_WIN32) || defined(_WIN32_) || defined(WIN64) || defined(_WIN64) || defined(_WIN64_)
    #include <ws2ipdef.h>
    #include <WS2tcpip.h>
#endif
#include <string.h>

namespace galay::async
{

HandleOption::HandleOption(GHandle handle)
{
    this->m_handle = handle;
}

bool HandleOption::HandleBlock()
{
#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    int flag = fcntl(this->m_handle.fd, F_GETFL, 0);
    flag &= ~O_NONBLOCK;
    int ret = fcntl(this->m_handle.fd, F_SETFL, flag);
#elif defined(WIN32) || defined(_WIN32) || defined(_WIN32_) || defined(WIN64) || defined(_WIN64) || defined(_WIN64_)
    u_long mode = 0; // 1 表示非阻塞模式
    int ret = ioctlsocket(m_handle, FIONBIO, &mode);
#endif
    if (ret < 0) {
        m_error_code = error::MakeErrorCode(error::ErrorCode::Error_SetBlockError, errno);
        return false;
    }
    return true;
}

bool HandleOption::HandleNonBlock()
{
#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    int flag = fcntl(this->m_handle.fd, F_GETFL, 0);
    flag |= O_NONBLOCK;
    int ret = fcntl(this->m_handle.fd, F_SETFL, flag);
#elif defined(WIN32) || defined(_WIN32) || defined(_WIN32_) || defined(WIN64) || defined(_WIN64) || defined(_WIN64_)
    u_long mode = 1; // 1 表示非阻塞模式
    int ret = ioctlsocket(m_handle.fd, FIONBIO, &mode);
#endif
    if (ret < 0) {
        m_error_code = error::MakeErrorCode(error::ErrorCode::Error_SetNoBlockError, errno);
        return false;
    }
    return true;
}

bool HandleOption::HandleReuseAddr()
{
#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    int option = 1;
    int ret = setsockopt(this->m_handle.fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
#elif  defined(WIN32) || defined(_WIN32) || defined(_WIN32_) || defined(WIN64) || defined(_WIN64) || defined(_WIN64_)
    BOOL option = TRUE;
    int ret = setsockopt(m_handle.fd, SOL_SOCKET, SO_REUSEADDR, (char*)&option, sizeof(option));
#endif
    if (ret < 0) {   
        m_error_code = error::MakeErrorCode(error::ErrorCode::Error_SetSockOptError, errno);
        return false;
    }
    return true;
}

bool HandleOption::HandleReusePort()
{
#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    int option = 1;
    int ret = setsockopt(this->m_handle.fd, SOL_SOCKET, SO_REUSEPORT, &option, sizeof(option));
    if (ret < 0) {
        m_error_code = error::MakeErrorCode(error::ErrorCode::Error_SetSockOptError, errno);
        return false;
    }
#elif  defined(WIN32) || defined(_WIN32) || defined(_WIN32_) || defined(WIN64) || defined(_WIN64) || defined(_WIN64_)
    //To Do
#endif
    return true;
}

uint32_t& HandleOption::GetErrorCode()
{
    return m_error_code;
}

bool AsyncSocket(AsyncTcpSocket *asocket)
{
    asocket->GetErrorCode() = error::ErrorCode::Error_NoError;
    asocket->GetHandle().fd = socket(AF_INET, SOCK_STREAM, 0);
    if (asocket->GetHandle().fd < 0) {
        asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_SocketError, errno);
        return false;
    }
    return true;
}

bool BindAndListen(AsyncTcpSocket *asocket, int port, int backlog)
{
    asocket->GetErrorCode() = error::ErrorCode::Error_NoError;
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if( bind(asocket->GetHandle().fd, (sockaddr*)&addr, sizeof(addr)) )
    {
        asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_BindError, errno);
        return false;
    }
    if( listen(asocket->GetHandle().fd, backlog) )
    {
        asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_ListenError, errno);
        return false;
    }
    return true;
}

coroutine::Awaiter_GHandle AsyncAccept(AsyncTcpSocket *asocket, NetAddr* addr)
{
    asocket->GetAction()->GetBindEvent()->ResetNetWaitEventType(event::kWaitEventTypeAccept);
    asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return coroutine::Awaiter_GHandle(asocket->GetAction(), addr);  
}

coroutine::Awaiter_bool AsyncConnect(AsyncTcpSocket *asocket, NetAddr* addr)
{
    asocket->GetAction()->GetBindEvent()->ResetNetWaitEventType(event::kWaitEventTypeConnect);
    asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return coroutine::Awaiter_bool(asocket->GetAction(), addr);
}

coroutine::Awaiter_int AsyncRecv(AsyncTcpSocket *asocket, IOVec* iov)
{
    asocket->GetAction()->GetBindEvent()->ResetNetWaitEventType(event::kWaitEventTypeRecv);
    asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return coroutine::Awaiter_int(asocket->GetAction(), iov);
}

coroutine::Awaiter_int AsyncSend(AsyncTcpSocket *asocket, IOVec *iov)
{
    asocket->GetAction()->GetBindEvent()->ResetNetWaitEventType(event::kWaitEventTypeSend);
    asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return coroutine::Awaiter_int(asocket->GetAction(), iov);
}

coroutine::Awaiter_bool AsyncClose(AsyncTcpSocket *asocket)
{
    asocket->GetAction()->GetBindEvent()->ResetNetWaitEventType(event::kWaitEventTypeClose);
    asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return coroutine::Awaiter_bool(asocket->GetAction(), nullptr);
}

bool AsyncSSLSocket(AsyncTcpSslSocket* asocket, SSL_CTX *ctx)
{
    SSL* ssl = SSL_new(ctx);
    if(ssl == nullptr) return false;
    if(SSL_set_fd(ssl, asocket->GetHandle().fd)) {
        asocket->GetSSL() = ssl;
        return true;
    }
    SSL_free(ssl);
    return false;
}

coroutine::Awaiter_bool AsyncSSLAccept(AsyncTcpSslSocket *asocket)
{
    asocket->GetAction()->GetBindEvent()->ResetNetWaitEventType(event::kWaitEventTypeSslAccept);
    asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return coroutine::Awaiter_bool(asocket->GetAction(), nullptr);
}

coroutine::Awaiter_bool SSLConnect(AsyncTcpSslSocket *asocket)
{
    asocket->GetAction()->GetBindEvent()->ResetNetWaitEventType(event::kWaitEventTypeSslConnect);
    asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return coroutine::Awaiter_bool(asocket->GetAction(), nullptr);
}

coroutine::Awaiter_int AsyncSSLRecv(AsyncTcpSslSocket *asocket, IOVec *iov)
{
    asocket->GetAction()->GetBindEvent()->ResetNetWaitEventType(event::kWaitEventTypeSslRecv);
    asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return coroutine::Awaiter_int(asocket->GetAction(), iov);
}

coroutine::Awaiter_int AsyncSSLSend(AsyncTcpSslSocket *asocket, IOVec *iov)
{
    asocket->GetAction()->GetBindEvent()->ResetNetWaitEventType(event::kWaitEventTypeSslSend);
    asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return coroutine::Awaiter_int(asocket->GetAction(), iov);
}

coroutine::Awaiter_bool AsyncSSLClose(AsyncTcpSslSocket *asocket)
{
    asocket->GetAction()->GetBindEvent()->ResetNetWaitEventType(event::kWaitEventTypeSslClose);
    asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return coroutine::Awaiter_bool(asocket->GetAction(), nullptr);
}

AsyncTcpSocket::AsyncTcpSocket(event::EventEngine* engine)
{
    event::TcpWaitEvent* event = new event::TcpWaitEvent(this);
    this->m_action = new action::TcpEventAction(engine, event);
}

HandleOption
AsyncTcpSocket::GetOption()
{
    return HandleOption(this->m_handle);
}

AsyncTcpSocket::~AsyncTcpSocket()
{
    delete m_action;
}

uint32_t &AsyncTcpSocket::GetErrorCode()
{
    return this->m_err_code;
}

AsyncTcpSslSocket::AsyncTcpSslSocket(event::EventEngine *engine)
    : AsyncTcpSocket(engine)
{
}

AsyncTcpSslSocket::~AsyncTcpSslSocket()
{
    if(m_ssl) {
        SSL_shutdown(m_ssl);
        SSL_free(m_ssl);
    }
}

AsyncUdpSocket::AsyncUdpSocket()
{
}

HandleOption AsyncUdpSocket::GetOption()
{
    return HandleOption(m_handle);
}

coroutine::Awaiter_bool AsyncUdpSocket::Socket()
{
    m_handle.fd = socket(AF_INET, SOCK_DGRAM, 0);
    if( this->m_handle.fd < 0 ) {
        m_error_code = error::MakeErrorCode(error::ErrorCode::Error_SocketError, errno);
        return coroutine::Awaiter_bool(false);
    }
    m_error_code = error::ErrorCode::Error_NoError;
    return coroutine::Awaiter_bool(true);
}

coroutine::Awaiter_bool AsyncUdpSocket::Close()
{
#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    int ret = close(this->m_handle.fd);
#elif defined(WIN32) || defined(_WIN32) || defined(_WIN32_) || defined(WIN64) || defined(_WIN64) || defined(_WIN64_)
    int ret = closesocket(this->m_handle);
#endif
    if( ret < 0 ) {
        m_error_code = error::MakeErrorCode(error::ErrorCode::Error_CloseError, errno);
        return coroutine::Awaiter_bool(false);
    }
    m_error_code = error::ErrorCode::Error_NoError;
    return coroutine::Awaiter_bool(true);
}

AsyncUdpSocket::~AsyncUdpSocket()
{
}

}