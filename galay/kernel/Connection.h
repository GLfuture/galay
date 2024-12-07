#ifndef GALAY_OPERATION_H
#define GALAY_OPERATION_H

#include <any>
#include <functional>
#include "galay/protocol/Http.h"
#include "galay/util/ObjectPool.hpp"
#include "Event.h"

namespace galay::coroutine
{   
    class Coroutine;
    class Awaiter_int;
    class Awaiter_bool;
};

namespace galay::async
{
    class AsyncNetIo;
};

namespace galay::details
{
    class IOEventAction;
    class EventEngine;
};


namespace galay
{

class TcpConnection
{
public:
    using ptr = std::shared_ptr<TcpConnection>;
    explicit TcpConnection(async::AsyncNetIo* socket);
    [[nodiscard]] async::AsyncNetIo* GetSocket() const { return m_socket; } 
    
    ~TcpConnection();
private:
    async::AsyncNetIo* m_socket;
};

class TcpConnectionManager
{
public:
    TcpConnectionManager(async::AsyncNetIo* action);
    TcpConnection::ptr GetConnection();
    std::shared_ptr<std::any> GetContext();
    ~TcpConnectionManager();
private:
    std::shared_ptr<std::any> m_context;
    TcpConnection::ptr m_connection;
};

class TcpSslConnection
{
public:
    using ptr = std::shared_ptr<TcpSslConnection>;
    explicit TcpSslConnection(async::AsyncSslNetIo* socket);
    [[nodiscard]] async::AsyncSslNetIo* GetSocket() const { return m_socket; }    
    ~TcpSslConnection();
private:
    async::AsyncSslNetIo* m_socket;
};

class TcpSslConnectionManager
{
public:
    TcpSslConnectionManager(async::AsyncSslNetIo* socket);
    TcpSslConnection::ptr GetConnection();
    std::shared_ptr<std::any> GetContext();
    ~TcpSslConnectionManager();
private:
    std::shared_ptr<std::any> m_context;
    TcpSslConnection::ptr m_connection;
};

class TcpCallbackStore
{
public:
    explicit TcpCallbackStore(const std::function<coroutine::Coroutine(TcpConnectionManager)>& callback);
    void Execute(async::AsyncNetIo* socket);
private:
    std::function<coroutine::Coroutine(TcpConnectionManager)> m_callback;
};

class TcpSslCallbackStore
{
public:
    explicit TcpSslCallbackStore(const std::function<coroutine::Coroutine(TcpSslConnectionManager)>& callback);
    void Execute(async::AsyncSslNetIo* socket);
private:
    std::function<coroutine::Coroutine(TcpSslConnectionManager)> m_callback;
};

template <typename T>
concept RequestType = std::is_base_of<protocol::Request, T>::value;

template <typename T>
concept ResponseType = std::is_base_of<protocol::Response, T>::value;

template <RequestType Request, ResponseType Response>
struct ProtocolStore
{
    using ptr = std::shared_ptr<ProtocolStore>;
    ProtocolStore(util::ObjectPoolMutiThread<Request>* request_pool, \
        util::ObjectPoolMutiThread<Response>* response_pool)
        :m_request(request_pool->GetObjector()), m_response(response_pool->GetObjector()), m_request_pool(request_pool), m_response_pool(response_pool) {}
    ~ProtocolStore() {
        m_request_pool->ReturnObjector(m_request);
        m_response_pool->ReturnObjector(m_response);
    }
    Request* m_request;
    Response* m_response;
    util::ObjectPoolMutiThread<Request>* m_request_pool;
    util::ObjectPoolMutiThread<Response>* m_response_pool;
};

template <RequestType Request, ResponseType Response>
class ConnectionManager
{
public:

    ConnectionManager(const TcpConnectionManager& manager, util::ObjectPoolMutiThread<Request>* request_pool, \
        util::ObjectPoolMutiThread<Response>* response_pool)
        :m_tcp_manager(manager), m_proto_store(std::make_shared<ProtocolStore<Request, Response>>(request_pool, response_pool)) {}

    Request* GetRequest() const { return m_proto_store->m_request; }
    Response* GetResponse() const { return m_proto_store->m_response; }
    TcpConnection::ptr GetConnection() { return m_tcp_manager.GetConnection(); }
    ~ConnectionManager() = default;
private:
    TcpConnectionManager m_tcp_manager;
    ProtocolStore<Request, Response>::ptr m_proto_store;
};

template <RequestType Request, ResponseType Response>
class SslConnectionManager
{
public:
    SslConnectionManager(const TcpSslConnectionManager& manager, util::ObjectPoolMutiThread<Request>* request_pool, \
        util::ObjectPoolMutiThread<Response>* response_pool)
        :m_tcp_ssl_manager(manager), m_proto_store(std::make_shared<ProtocolStore<Request, Response>>(request_pool, response_pool)) {}
    Request* GetRequest() const { return m_proto_store->m_request; }
    Response* GetResponse() const { return m_proto_store->m_response; }
    TcpSslConnection::ptr GetConnection() { return m_tcp_ssl_manager.GetConnection(); }
    ~SslConnectionManager() = default;
private:
    TcpSslConnectionManager m_tcp_ssl_manager;
    ProtocolStore<Request, Response>::ptr m_proto_store;
};

using HttpConnectionManager = ConnectionManager<protocol::http::HttpRequest, protocol::http::HttpResponse>;
using HttpsConnectionManager = SslConnectionManager<protocol::http::HttpRequest, protocol::http::HttpResponse>;


}


#endif