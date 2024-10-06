#include "Async.h"
#include "Event.h"
#include "EventEngine.h"
#if defined(__linux__)
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <fcntl.h>
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
    int flag = fcntl(this->m_handle, F_GETFL, 0);
    flag &= ~O_NONBLOCK;
    int ret = fcntl(this->m_handle, F_SETFL, flag);
    if (ret < 0) {
        return false;
    }
    return true;
}

bool HandleOption::HandleNonBlock()
{
    int flag = fcntl(this->m_handle, F_GETFL, 0);
    flag |= O_NONBLOCK;
    int ret = fcntl(this->m_handle, F_SETFL, flag);
    if (ret < 0) {
        return false;
    }
    
    return true;
}

bool HandleOption::HandleReuseAddr()
{
    int option = 1;
    int ret = setsockopt(this->m_handle, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    if (ret < 0) {   
        return false;
    }
    return true;
}

bool HandleOption::HandleReusePort()
{
    int option = 1;
    int ret = setsockopt(this->m_handle, SOL_SOCKET, SO_REUSEPORT, &option, sizeof(option));
    if (ret < 0) {
        return false;
    }
    return true;
}

std::string HandleOption::GetLastError()
{
    return strerror(errno);
}

AsyncTcpSocket::AsyncTcpSocket(GHandle handle, event::EventEngine *engine)
    : m_handle(handle), m_engine(engine), m_handle_closed(true)
{
}

AsyncTcpSocket::AsyncTcpSocket(event::EventEngine *engine)
    : m_engine(engine), m_handle_closed(true)
{
}

HandleOption
AsyncTcpSocket::GetOption()
{
    return HandleOption(this->m_handle);
}

coroutine::Awaiter_bool AsyncTcpSocket::InitialHandle()
{
    this->m_handle = socket(AF_INET, SOCK_STREAM, 0);
    if( this->m_handle < 0 ) {
        this->m_last_error = "Socket failed";
        return coroutine::Awaiter_bool(false);
    }
    if( !m_last_error.empty() ) m_last_error.clear();
    m_handle_closed = false;
    return coroutine::Awaiter_bool(true);
}

coroutine::Awaiter_bool AsyncTcpSocket::InitialHandle(GHandle handle)
{
    sockaddr addr;
    socklen_t addrlen = sizeof(addr);
    this->m_handle = accept(handle, &addr, &addrlen);
    if( this->m_handle < 0 ) {
        if( errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR )
        {
            if( !m_last_error.empty() ) m_last_error.clear();
            return coroutine::Awaiter_bool(false);
        }else {
            this->m_last_error = "Accept failed";
            return coroutine::Awaiter_bool{false};
        }
    }
    if( !m_last_error.empty() ) m_last_error.clear();
    m_handle_closed = false;
    return coroutine::Awaiter_bool(true);
}

coroutine::Awaiter_bool AsyncTcpSocket::Connect(const char *host, int port)
{
    if( !m_last_error.empty() ) m_last_error.clear();
    //return coroutine::Awaiter_int();
}

coroutine::Awaiter_int AsyncTcpSocket::Recv(action::WaitAction* action)
{
    
    if( !m_last_error.empty() ) m_last_error.clear();
    return coroutine::Awaiter_int(action);
}

coroutine::Awaiter_int AsyncTcpSocket::Send(action::WaitAction* action)
{
    if( !m_last_error.empty() ) m_last_error.clear();
    return coroutine::Awaiter_int(action);
}

coroutine::Awaiter_bool AsyncTcpSocket::Close()
{

    if( m_handle_closed ) return coroutine::Awaiter_bool(false);
    int ret = close(this->m_handle);
    if( ret < 0 ) {
        this->m_last_error = "Close failed";
        return coroutine::Awaiter_bool(false);
    }
    if( !m_last_error.empty() ) m_last_error.clear();
    m_handle_closed = true;
    return coroutine::Awaiter_bool(true);
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
    if( ! m_handle_closed ) {
        Close();
    }
}

std::string AsyncTcpSocket::GetRemoteAddr()
{
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    // 获取套接字的地址信息
    if (getsockname(m_handle, (struct sockaddr*)&addr, &addr_len) == -1) {
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
    if (getsockname(m_handle, (struct sockaddr*)&addr, &addr_len) == -1) {
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

    
}