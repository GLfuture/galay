#ifndef GALAY_CLIENT_TCC
#define GALAY_CLIENT_TCC

#include "Client.hpp"


namespace galay
{

template <typename SocketType>
inline TcpClient<SocketType>::TcpClient(GHandle handle)
{
    if(handle.fd == -1) {
        m_socket = std::make_unique<SocketType>();
        m_socket->Socket();
    } else {
        m_socket = std::make_unique<SocketType>(handle);
    }
}

template <typename SocketType>
inline THost TcpClient<SocketType>::GetRemoteAddr() const
{
    return m_remote;
}

template <typename SocketType>
inline uint32_t TcpClient<SocketType>::GetErrorCode() const
{
    return m_socket->GetErrorCode();
}

template <typename SocketType>
inline std::string TcpClient<SocketType>::GetErrorString() const
{
    return error::GetErrorString(m_socket->GetErrorCode());
}

template <typename SocketType>
template <typename CoRtn>
inline AsyncResult<bool, CoRtn> TcpClient<SocketType>::Connect(THost *addr, int64_t timeout)
{
    m_remote = *addr;
    return m_socket->template Connect<CoRtn>(&m_remote, timeout);
}

template <typename SocketType>
template <typename CoRtn>
inline AsyncResult<int, CoRtn> TcpClient<SocketType>::Recv(TcpIOVecHolder &holder, size_t length, int64_t timeout)
{
    return m_socket->template Recv<CoRtn>(holder, length, timeout);
}

template <typename SocketType>
template <typename CoRtn>
inline AsyncResult<int, CoRtn> TcpClient<SocketType>::Send(TcpIOVecHolder &holder, size_t length, int64_t timeout)
{
    return m_socket->template Send<CoRtn>(holder, length, timeout);
}



template <typename SocketType>
template <typename CoRtn>
inline AsyncResult<bool, CoRtn> TcpClient<SocketType>::Close()
{
    return m_socket->template Close<CoRtn>();
}
}

#endif