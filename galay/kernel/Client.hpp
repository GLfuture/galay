#ifndef GALAY_CLIENT_HPP
#define GALAY_CLIENT_HPP

#include "Async.hpp"

namespace galay
{

template<typename SocketType>
class TcpClient
{
public:
    using ptr = std::shared_ptr<TcpClient>;
    using uptr = std::unique_ptr<TcpClient>;

    TcpClient(GHandle handle = {});

    template<typename CoRtn = void>
    AsyncResult<bool, CoRtn> Connect(THost* addr, int64_t timeout = -1);

    template<typename CoRtn = void>
    AsyncResult<int, CoRtn> Recv(TcpIOVecHolder& holder, size_t length, int64_t timeout = -1);
    template<typename CoRtn = void>
    AsyncResult<int, CoRtn> Send(TcpIOVecHolder& holder, size_t length, int64_t timeout = -1);

    template<typename CoRtn = void>
    AsyncResult<bool, CoRtn> Close();

    THost GetRemoteAddr() const;

    uint32_t GetErrorCode() const;
    std::string GetErrorString() const;

private:
    THost m_remote;
    std::unique_ptr<SocketType> m_socket;
};

}

#include "Client.tcc"

#endif