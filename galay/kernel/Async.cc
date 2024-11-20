#include "Async.h"
#include "Event.h"
#include "EventEngine.h"
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

AsyncTcpSocket::AsyncTcpSocket()
{
}

AsyncTcpSocket::AsyncTcpSocket(GHandle handle)
    :m_handle(handle)
{
}

HandleOption
AsyncTcpSocket::GetOption()
{
    return HandleOption(this->m_handle);
}

GHandle AsyncTcpSocket::Socket()
{
    return GHandle{
        .fd = socket(AF_INET, SOCK_STREAM, 0)
    };
}

coroutine::Awaiter_GHandle AsyncTcpSocket::Accept(action::TcpEventAction *action)
{
    action->GetBindEvent()->ResetNetWaitEventType(event::kWaitEventTypeAccept);
    m_err_code = error::ErrorCode::Error_NoError;
    return coroutine::Awaiter_GHandle(action, nullptr);
}

bool AsyncTcpSocket::BindAndListen(int port, int backlog)
{
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if( bind(this->m_handle.fd, (sockaddr*)&addr, sizeof(addr)) )
    {
        this->m_err_code = error::MakeErrorCode(error::ErrorCode::Error_BindError, errno);
        return false;
    }
    if( listen(this->m_handle.fd, backlog) )
    {
        this->m_err_code = error::MakeErrorCode(error::ErrorCode::Error_ListenError, errno);
        return false;
    }
    this->m_err_code = error::ErrorCode::Error_NoError;
    return true;
}

coroutine::Awaiter_bool AsyncTcpSocket::Connect(action::TcpEventAction* action, NetAddr* addr)
{
    action->GetBindEvent()->ResetNetWaitEventType(event::kWaitEventTypeConnect);
    this->m_err_code = error::ErrorCode::Error_NoError;
    return coroutine::Awaiter_bool(action, addr);
}

coroutine::Awaiter_int AsyncTcpSocket::Recv(action::TcpEventAction * action)
{
    action->GetBindEvent()->ResetNetWaitEventType(event::kWaitEventTypeRecv);
    this->m_err_code = error::ErrorCode::Error_NoError;
    return coroutine::Awaiter_int(action, nullptr);
}

coroutine::Awaiter_int AsyncTcpSocket::Send(action::TcpEventAction* action)
{
    action->GetBindEvent()->ResetNetWaitEventType(event::kWaitEventTypeSend);
    this->m_err_code = error::ErrorCode::Error_NoError;
    return coroutine::Awaiter_int(action, nullptr);
}

coroutine::Awaiter_bool AsyncTcpSocket::Close(action::TcpEventAction* action)
{
    action->GetBindEvent()->ResetNetWaitEventType(event::kWaitEventTypeClose);
    this->m_err_code = error::ErrorCode::Error_NoError;
    return coroutine::Awaiter_bool(action, nullptr);
}


AsyncTcpSocket::~AsyncTcpSocket()
{
}

std::string AsyncTcpSocket::GetRemoteAddr()
{
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    // 获取套接字的地址信息
    if (getsockname(m_handle.fd, (struct sockaddr*)&addr, &addr_len) == -1) {
        this->m_err_code = error::MakeErrorCode(error::ErrorCode::Error_GetSockNameError, errno);
        return "";
    }

    // 将IP地址转换为字符串
    char ip_str[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &addr.sin_addr, ip_str, sizeof(ip_str)) == nullptr) {
        this->m_err_code = error::MakeErrorCode(error::ErrorCode::Error_InetNtopError, errno);
        return "";
    }
    this->m_err_code = error::ErrorCode::Error_NoError;
    return std::string(ip_str);
}

uint32_t &AsyncTcpSocket::GetErrorCode()
{
    return this->m_err_code;
}

int AsyncTcpSocket::GetRemotePort()
{
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    // 获取套接字的地址信息
    if (getsockname(m_handle.fd, (struct sockaddr*)&addr, &addr_len) == -1) {
        this->m_err_code = error::MakeErrorCode(error::ErrorCode::Error_GetSockNameError, errno);
        return -1;
    }

    // 将IP地址转换为字符串
    char ip_str[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &addr.sin_addr, ip_str, sizeof(ip_str)) == nullptr) {
        this->m_err_code = error::MakeErrorCode(error::ErrorCode::Error_InetNtopError, errno);
        return -1;
    }
    
    this->m_err_code = error::ErrorCode::Error_NoError;
    return ntohs(addr.sin_port);
}

AsyncTcpSslSocket::AsyncTcpSslSocket()
    :m_ssl(nullptr)
{
}

AsyncTcpSslSocket::AsyncTcpSslSocket(GHandle handle, SSL *ssl)
    : AsyncTcpSocket(handle), m_ssl(ssl)
{
}

SSL* AsyncTcpSslSocket::SSLSocket(GHandle& handle, SSL_CTX* ctx)
{
    SSL* ssl = SSL_new(ctx);
    if(ssl == nullptr) return nullptr;
    if(SSL_set_fd(ssl, handle.fd)) {
        return ssl;
    }
    SSL_free(ssl);
    return nullptr;
}


coroutine::Awaiter_GHandle AsyncTcpSslSocket::Accept(action::TcpSslEventAction *action)
{
    return AsyncTcpSocket::Accept(action);
}

coroutine::Awaiter_bool AsyncTcpSslSocket::SSLAccept(action::TcpSslEventAction *action)
{
    action->GetBindEvent()->ResetNetWaitEventType(event::kWaitEventTypeSslAccept);
    this->m_err_code = error::ErrorCode::Error_NoError;
    return coroutine::Awaiter_bool(action, nullptr);
}

bool AsyncTcpSslSocket::BindAndListen(int port, int backlog)
{
    return AsyncTcpSocket::BindAndListen(port, backlog);
}

coroutine::Awaiter_bool AsyncTcpSslSocket::Connect(action::TcpSslEventAction *action, NetAddr* addr)
{
    return AsyncTcpSocket::Connect(action, addr);
}

coroutine::Awaiter_int AsyncTcpSslSocket::SSLRecv(action::TcpSslEventAction *action)
{
    action->GetBindEvent()->ResetNetWaitEventType(event::kWaitEventTypeSslRecv);
    this->m_err_code = error::ErrorCode::Error_NoError;
    return coroutine::Awaiter_int(action, nullptr);
}

coroutine::Awaiter_int AsyncTcpSslSocket::SSLSend(action::TcpSslEventAction *action)
{
    action->GetBindEvent()->ResetNetWaitEventType(event::kWaitEventTypeSslSend);
    this->m_err_code = error::ErrorCode::Error_NoError;
    return coroutine::Awaiter_int(action, nullptr);
}

coroutine::Awaiter_bool AsyncTcpSslSocket::SSLClose(action::TcpSslEventAction *action)
{
    action->GetBindEvent()->ResetNetWaitEventType(event::kWaitEventTypeSslClose);
    this->m_err_code = error::ErrorCode::Error_NoError;
    return coroutine::Awaiter_bool(action, nullptr);
}

coroutine::Awaiter_bool AsyncTcpSslSocket::SSLConnect(action::TcpSslEventAction *action)
{
    action->GetBindEvent()->ResetNetWaitEventType(event::kWaitEventTypeSslConnect);
    this->m_err_code = error::ErrorCode::Error_NoError;
    return coroutine::Awaiter_bool(action, nullptr);
}

AsyncTcpSslSocket::~AsyncTcpSslSocket()
{
    if(m_ssl) {
        SSL_shutdown(m_ssl);
        SSL_free(m_ssl);
    }
}

GHandle AsyncTcpSslSocket::Socket()
{
    return AsyncTcpSocket::Socket();
}

coroutine::Awaiter_int AsyncTcpSslSocket::Recv(action::TcpEventAction *action)
{
    return AsyncTcpSocket::Recv(action);
}

coroutine::Awaiter_int AsyncTcpSslSocket::Send(action::TcpEventAction *action)
{
    return AsyncTcpSocket::Send(action);
}

coroutine::Awaiter_bool AsyncTcpSslSocket::Close(action::TcpEventAction *action)
{
    return AsyncTcpSocket::Close(action);
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