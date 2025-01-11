#ifndef GALAY_OPERATION_H
#define GALAY_OPERATION_H

#include <any>
#include <functional>
#include "Coroutine.hpp"
#include "galay/protocol/Http.h"
#include "galay/utils/Pool.hpp"

namespace galay
{


template <typename Socket>
class Connection
{
public:
    using ptr = std::shared_ptr<Connection>;
    explicit Connection(Socket* socket);
    
    template <typename CoRtn = void>
    AsyncResult<int, CoRtn> Recv(TcpIOVec *iov, int size);

    template <typename CoRtn = void>
    AsyncResult<int, CoRtn> Send(TcpIOVec *iov, int size);

    template <typename CoRtn = void>
    AsyncResult<bool, CoRtn> Close();

    ~Connection();
private:
    Socket* m_socket;
};


template <typename T>
concept RequestType = std::is_base_of<Request, T>::value;

template <typename T>
concept ResponseType = std::is_base_of<Response, T>::value;

template <typename Socket, RequestType Request, ResponseType Response>
class Session
{
public:
    using ptr = std::shared_ptr<Session>;
    Session(std::shared_ptr<Connection<Socket>> connection);

    std::shared_ptr<Connection<Socket>> GetConnection();
    Request* GetRequest() const;
    Response* GetResponse() const;
    void* GetUserData();
    void SetUserData(void* data);

    void Close();
    bool IsClose();

    ~Session() = default;
private:
    bool m_isClosed;
    void* m_userdata;
    Request* m_request;
    Response* m_response;
    std::shared_ptr<Connection<Socket>> m_connection;
};



using HttpSession = Session<AsyncTcpSocket, http::HttpRequest, http::HttpResponse>;
using HttpsSession = Session<AsyncTcpSslSocket, http::HttpRequest, http::HttpResponse>;

}

#include "Session.tcc"

#endif