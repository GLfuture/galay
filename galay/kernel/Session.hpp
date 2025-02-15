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
class Connection: public std::enable_shared_from_this<Connection<Socket>>
{
public:
    using ptr = std::shared_ptr<Connection>;
    using timeout_callback_t = std::function<void(Connection<Socket>::ptr)>;

    explicit Connection(Socket* socket);
    
    template <typename CoRtn = void>
    AsyncResult<int, CoRtn> Recv(TcpIOVec *iov, int size, int64_t timeout_ms);

    template <typename CoRtn = void>
    AsyncResult<int, CoRtn> Send(TcpIOVec *iov, int size, int64_t timeout_ms);

    template <typename CoRtn = void>
    AsyncResult<bool, CoRtn> Close();

    void Destroy();
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
    Session(Connection<Socket>::ptr connection);

    Connection<Socket>::ptr GetConnection();
    Request* GetRequest() const;
    Response* GetResponse() const;
    void* GetUserData();
    void SetUserData(void* data);

    void Close();
    bool IsClose();

    ~Session();
private:
    bool m_isClosed;
    void* m_userdata;
    Request* m_request;
    Response* m_response;
    Connection<Socket>::ptr m_connection;
};



using HttpSession = Session<AsyncTcpSocket, http::HttpRequest, http::HttpResponse>;
using HttpsSession = Session<AsyncTcpSslSocket, http::HttpRequest, http::HttpResponse>;



}

#include "Session.tcc"

#endif