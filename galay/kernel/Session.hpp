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
    explicit Connection(Socket* socket) 
        :m_socket(socket) {}
    
    [[nodiscard]] Socket* GetSocket() const { return m_socket; } 
    
    ~Connection() { delete m_socket; }
private:
    Socket* m_socket;
};


template <typename Socket>
class CallbackStore
{
public:
    explicit CallbackStore(const std::function<Coroutine<void>(RoutineCtx::ptr,std::shared_ptr<Connection<Socket>>)>& callback) 
        : m_callback(callback) {}
    void Execute(Socket* socket) {
        auto connection = std::make_shared<Connection<Socket>>(socket);
        m_callback(galay::RoutineCtx::Create(), connection);
    }
private:
    std::function<Coroutine<void>(RoutineCtx::ptr,std::shared_ptr<Connection<Socket>>)> m_callback;
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
        utils::ProtocolPool<Response>* response_pool)
        :m_request(request_pool->GetProtocol()), m_response(response_pool->GetProtocol()), \
            m_request_pool(request_pool), m_response_pool(response_pool) {}
    ~ProtocolStore() {
        m_request_pool->PutProtocol(std::move(m_request));
        m_response_pool->PutProtocol(std::move(m_response));
    }
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
        utils::ProtocolPool<Response>* response_pool)
        :m_connection(connection), \
            m_proto_store(std::make_shared<ProtocolStore<Request, Response>>(request_pool, response_pool)) {}

    Request* GetRequest() const { return m_proto_store->m_request.get(); }
    Response* GetResponse() const { return m_proto_store->m_response.get(); }
    std::shared_ptr<Connection<Socket>> GetConnection() { return m_connection; }
    void* GetUserData() { return m_userdata; }
    void SetUserData(void* data) { m_userdata = data; }

    void ToClose() { m_close = true; }
    bool IsClose() { return m_close; }
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


#endif