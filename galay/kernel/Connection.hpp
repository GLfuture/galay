#ifndef GALAY_OPERATION_H
#define GALAY_OPERATION_H

#include <any>
#include <functional>
#include "galay/protocol/Http.h"
#include "galay/utils/ObjectPool.hpp"

namespace galay::coroutine
{   
    class Coroutine;
};

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
    explicit CallbackStore(const std::function<coroutine::Coroutine(std::shared_ptr<Connection<Socket>>)>& callback) 
        : m_callback(callback) {}
    void Execute(Socket* socket) {
        auto connection = std::make_shared<Connection<Socket>>(socket);
        m_callback(connection);
    }
private:
    std::function<coroutine::Coroutine(std::shared_ptr<Connection<Socket>>)> m_callback;
};


template <typename T>
concept RequestType = std::is_base_of<protocol::Request, T>::value;

template <typename T>
concept ResponseType = std::is_base_of<protocol::Response, T>::value;

template <RequestType Request, ResponseType Response>
struct ProtocolStore
{
    using ptr = std::shared_ptr<ProtocolStore>;
    ProtocolStore(utils::ObjectPoolMutiThread<Request>* request_pool, \
        utils::ObjectPoolMutiThread<Response>* response_pool)
        :m_request(request_pool->GetObjector()), m_response(response_pool->GetObjector()), m_request_pool(request_pool), m_response_pool(response_pool) {}
    ~ProtocolStore() {
        m_request_pool->ReturnObjector(m_request);
        m_response_pool->ReturnObjector(m_response);
    }
    Request* m_request;
    Response* m_response;
    utils::ObjectPoolMutiThread<Request>* m_request_pool;
    utils::ObjectPoolMutiThread<Response>* m_response_pool;
};

template <typename Socket, RequestType Request, ResponseType Response>
class Session
{
public:

    Session(std::shared_ptr<Connection<Socket>> connection, utils::ObjectPoolMutiThread<Request>* request_pool, \
        utils::ObjectPoolMutiThread<Response>* response_pool)
        :m_connection(connection), m_proto_store(std::make_shared<ProtocolStore<Request, Response>>(request_pool, response_pool)) {}

    Request* GetRequest() const { return m_proto_store->m_request; }
    Response* GetResponse() const { return m_proto_store->m_response; }
    std::shared_ptr<Connection<Socket>> GetConnection() { return m_connection; }
    std::shared_ptr<std::any> GetUserData() { return m_userdata; }
    ~Session() = default;
private:
    std::shared_ptr<std::any> m_userdata;
    std::shared_ptr<Connection<Socket>> m_connection;
    ProtocolStore<Request, Response>::ptr m_proto_store;
};

}


#endif