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
    [[nodiscard]] Socket* GetSocket() const;
    ~Connection();
private:
    Socket* m_socket;
};


template <typename Socket>
class CallbackStore
{
public:
    explicit CallbackStore(const std::function<Coroutine<void>(RoutineCtx,std::shared_ptr<Connection<Socket>>)>& callback);
    void Execute(Socket* socket);
private:
    std::function<Coroutine<void>(RoutineCtx,std::shared_ptr<Connection<Socket>>)> m_callback;
};


template <typename T>
concept RequestType = std::is_base_of<Request, T>::value;

template <typename T>
concept ResponseType = std::is_base_of<Response, T>::value;

template <RequestType Request, ResponseType Response>
struct ProtocolStore
{
    using ptr = std::shared_ptr<ProtocolStore>;
    ProtocolStore(utils::ProtocolPool<Request>* request_pool, \
        utils::ProtocolPool<Response>* response_pool);
    ~ProtocolStore();

    std::unique_ptr<Request> m_request;
    std::unique_ptr<Response> m_response;
    utils::ProtocolPool<Request>* m_request_pool;
    utils::ProtocolPool<Response>* m_response_pool;
};

template <typename Socket, RequestType Request, ResponseType Response>
class Session
{
public:
    Session(std::shared_ptr<Connection<Socket>> connection, utils::ProtocolPool<Request>* request_pool, \
        utils::ProtocolPool<Response>* response_pool);

    Request* GetRequest() const;
    Response* GetResponse() const;
    std::shared_ptr<Connection<Socket>> GetConnection();
    void* GetUserData();
    void SetUserData(void* data);

    void ToClose();
    bool IsClose();
    ~Session() = default;
private:
    void* m_userdata;
    bool m_close = false;
    std::shared_ptr<Connection<Socket>> m_connection;
    ProtocolStore<Request, Response>::ptr m_proto_store;
};



using HttpSession = Session<AsyncTcpSocket, http::HttpRequest, http::HttpResponse>;
using HttpsSession = Session<AsyncTcpSslSocket, http::HttpRequest, http::HttpResponse>;

}

#include "Session.tcc"

#endif