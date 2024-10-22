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
#include <spdlog/spdlog.h>
#include <string.h>

namespace galay::async
{

HandleOption::HandleOption(GHandle handle)
{
    this->m_handle = handle;
}

bool HandleOption::HandleBlock()
{
#if defined(__linux__)
    int flag = fcntl(this->m_handle.fd, F_GETFL, 0);
    flag &= ~O_NONBLOCK;
    int ret = fcntl(this->m_handle.fd, F_SETFL, flag);
#elif defined(WIN32) || defined(_WIN32) || defined(_WIN32_) || defined(WIN64) || defined(_WIN64) || defined(_WIN64_)
    u_long mode = 0; // 1 表示非阻塞模式
    int ret = ioctlsocket(m_handle, FIONBIO, &mode);
#endif
    if (ret < 0) {
        return false;
    }
    return true;
}

bool HandleOption::HandleNonBlock()
{
#if defined(__linux__)
    int flag = fcntl(this->m_handle.fd, F_GETFL, 0);
    flag |= O_NONBLOCK;
    int ret = fcntl(this->m_handle.fd, F_SETFL, flag);
#elif defined(WIN32) || defined(_WIN32) || defined(_WIN32_) || defined(WIN64) || defined(_WIN64) || defined(_WIN64_)
    u_long mode = 1; // 1 表示非阻塞模式
    int ret = ioctlsocket(m_handle.fd, FIONBIO, &mode);
#endif
    if (ret < 0) {
        return false;
    }
    return true;
}

bool HandleOption::HandleReuseAddr()
{
#if defined(__linux__)
    int option = 1;
    int ret = setsockopt(this->m_handle.fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
#elif  defined(WIN32) || defined(_WIN32) || defined(_WIN32_) || defined(WIN64) || defined(_WIN64) || defined(_WIN64_)
    BOOL option = TRUE;
    int ret = setsockopt(m_handle.fd, SOL_SOCKET, SO_REUSEADDR, (char*)&option, sizeof(option));
#endif
    if (ret < 0) {   
        return false;
    }
    return true;
}

bool HandleOption::HandleReusePort()
{
#if defined(__linux__)
    int option = 1;
    int ret = setsockopt(this->m_handle.fd, SOL_SOCKET, SO_REUSEPORT, &option, sizeof(option));
    if (ret < 0) {
        return false;
    }
#elif  defined(WIN32) || defined(_WIN32) || defined(_WIN32_) || defined(WIN64) || defined(_WIN64) || defined(_WIN64_)
    //To Do
#endif
    return true;
}

std::string HandleOption::GetLastError()
{
    return strerror(errno);
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

coroutine::Awaiter_bool AsyncTcpSocket::InitialHandle(action::NetIoEventAction* action)
{
    spdlog::debug("AsyncTcpSocket::InitialHandle");
    action->GetBindEvent()->ResetNetWaitEventType(event::NetWaitEvent::kWaitEventTypeSocket);
    if( !m_last_error.empty() ) m_last_error.clear();
    return coroutine::Awaiter_bool(action);
}

coroutine::Awaiter_GHandle AsyncTcpSocket::Accept(action::NetIoEventAction* action)
{
    spdlog::debug("AsyncTcpSocket::Accept");
    action->GetBindEvent()->ResetNetWaitEventType(event::NetWaitEvent::kWaitEventTypeAccept);
    if( !m_last_error.empty() ) m_last_error.clear();
    return coroutine::Awaiter_GHandle(action);
}

coroutine::Awaiter_bool AsyncTcpSocket::BindAndListen(int port, int backlog)
{
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if( bind(this->m_handle.fd, (sockaddr*)&addr, sizeof(addr)) )
    {
        this->m_last_error = "Bind failed";
        return coroutine::Awaiter_bool(false);
    }
    if( listen(this->m_handle.fd, backlog) )
    {
        this->m_last_error = "Listen failed";
        return coroutine::Awaiter_bool(false);
    }
    if( !m_last_error.empty() ) m_last_error.clear();
    return coroutine::Awaiter_bool(true);
}

coroutine::Awaiter_bool AsyncTcpSocket::Connect(action::NetIoEventAction* action, const NetAddr& addr)
{
    spdlog::debug("AsyncTcpSocket::Connect");
    this->m_connect_addr = addr;
    action->GetBindEvent()->ResetNetWaitEventType(event::NetWaitEvent::kWaitEventTypeConnect);
    if( !m_last_error.empty() ) m_last_error.clear();
    return coroutine::Awaiter_bool(action);
}

coroutine::Awaiter_int AsyncTcpSocket::Recv(action::NetIoEventAction * action)
{
    spdlog::debug("AsyncTcpSocket::Recv");
    action->GetBindEvent()->ResetNetWaitEventType(event::NetWaitEvent::kWaitEventTypeRecv);
    if( !m_last_error.empty() ) m_last_error.clear();
    return coroutine::Awaiter_int(action);
}

coroutine::Awaiter_int AsyncTcpSocket::Send(action::NetIoEventAction* action)
{
    spdlog::debug("AsyncTcpSocket::Send");
    action->GetBindEvent()->ResetNetWaitEventType(event::NetWaitEvent::kWaitEventTypeSend);
    if( !m_last_error.empty() ) m_last_error.clear();
    return coroutine::Awaiter_int(action);
}

coroutine::Awaiter_bool AsyncTcpSocket::Close(action::NetIoEventAction* action)
{
    spdlog::debug("AsyncTcpSocket::Close");
    action->GetBindEvent()->ResetNetWaitEventType(event::NetWaitEvent::kWaitEventTypeClose);
    if( !m_last_error.empty() ) m_last_error.clear();
    return coroutine::Awaiter_bool(action);
}


void AsyncTcpSocket::SetLastError(std::string error)
{
    this->m_last_error = error;
}

std::string AsyncTcpSocket::GetLastError()
{
    return this->m_last_error;
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
        this->m_last_error = "getsockname falied";
        return "";
    }

    // 将IP地址转换为字符串
    char ip_str[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &addr.sin_addr, ip_str, sizeof(ip_str)) == nullptr) {
        this->m_last_error = "inet_ntop falied";
        return "";
    }

    if( !m_last_error.empty() ) m_last_error.clear();
    return std::string(ip_str);
}

int AsyncTcpSocket::GetRemotePort()
{
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    // 获取套接字的地址信息
    if (getsockname(m_handle.fd, (struct sockaddr*)&addr, &addr_len) == -1) {
        this->m_last_error = "getsockname falied";
        return -1;
    }

    // 将IP地址转换为字符串
    char ip_str[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &addr.sin_addr, ip_str, sizeof(ip_str)) == nullptr) {
        this->m_last_error = "inet_ntop falied";
        return -1;
    }
    
    if( !m_last_error.empty() ) m_last_error.clear();
    return ntohs(addr.sin_port);
}

AsyncUdpSocket::AsyncUdpSocket()
    : m_handle_closed(true)
{
}

HandleOption AsyncUdpSocket::GetOption()
{
    return HandleOption(m_handle);
}

coroutine::Awaiter_bool AsyncUdpSocket::InitialHandle()
{
    m_handle.fd = socket(AF_INET, SOCK_DGRAM, 0);
    if( this->m_handle.fd < 0 ) {
        this->m_last_error = "Socket failed";
        return coroutine::Awaiter_bool(false);
    }
    if( !m_last_error.empty() ) m_last_error.clear();
    m_handle_closed = false;
    return coroutine::Awaiter_bool(true);
}

coroutine::Awaiter_bool AsyncUdpSocket::Close()
{
    if( m_handle_closed ) return coroutine::Awaiter_bool(false);
#if defined(__linux__)
    int ret = close(this->m_handle.fd);
#elif defined(WIN32) || defined(_WIN32) || defined(_WIN32_) || defined(WIN64) || defined(_WIN64) || defined(_WIN64_)
    int ret = closesocket(this->m_handle);
#endif
    if( ret < 0 ) {
        this->m_last_error = "Close failed";
        return coroutine::Awaiter_bool(false);
    }
    if( !m_last_error.empty() ) m_last_error.clear();
    m_handle_closed = true;
    return coroutine::Awaiter_bool(true);
}

AsyncUdpSocket::~AsyncUdpSocket()
{
    if( ! m_handle_closed ) {
        Close();
    }
}

coroutine::Awaiter_bool AsyncTcpSslSocket::Connect(action::NetIoEventAction *action, const NetAddr &addr)
{
    return m_socket.Connect(action, addr);
}

coroutine::Awaiter_bool AsyncTcpSslSocket::SSLConnect(action::NetIoEventAction *action)
{
    //return coroutine::Awaiter_bool();
}

coroutine::Awaiter_GHandle AsyncTcpSslSocket::Accept(action::NetIoEventAction *action)
{
    return m_socket.Accept(action);
}
coroutine::Awaiter_bool AsyncTcpSslSocket::SSLAccept()
{
    //return coroutine::Awaiter_bool();
}

}