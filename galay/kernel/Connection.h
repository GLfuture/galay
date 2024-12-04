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

namespace galay::event
{
    class EventEngine;
}

namespace galay::action
{
    class IOEventAction;
};

namespace galay::protocol::http
{
    class HttpRequest;
    class HttpResponse;
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

struct HttpProtoStore
{
    using HttpRequest = protocol::http::HttpRequest;
    using HttpResponse = protocol::http::HttpResponse;
    using ptr = std::shared_ptr<HttpProtoStore>;

    HttpProtoStore(util::ObjectPoolMutiThread<HttpRequest>* request_pool, \
        util::ObjectPoolMutiThread<HttpResponse>* response_pool);
    protocol::http::HttpRequest* m_request;
    protocol::http::HttpResponse* m_response;
    util::ObjectPoolMutiThread<HttpRequest>* m_request_pool;
    util::ObjectPoolMutiThread<HttpResponse>* m_response_pool;
    ~HttpProtoStore();
};

class HttpConnectionManager
{
    using HttpRequest = protocol::http::HttpRequest;
    using HttpResponse = protocol::http::HttpResponse;
public:

    HttpConnectionManager(const TcpConnectionManager& manager, util::ObjectPoolMutiThread<HttpRequest>* request_pool, \
        util::ObjectPoolMutiThread<HttpResponse>* response_pool);
    protocol::http::HttpRequest* GetRequest() const;
    protocol::http::HttpResponse* GetResponse() const;
    TcpConnection::ptr GetConnection();
    ~HttpConnectionManager();
private:
    TcpConnectionManager m_tcp_manager;
    HttpProtoStore::ptr m_proto_store;
};

class HttpsConnectionManager
{
    using HttpRequest = protocol::http::HttpRequest;
    using HttpResponse = protocol::http::HttpResponse;
public:

    HttpsConnectionManager(const TcpSslConnectionManager& manager, util::ObjectPoolMutiThread<HttpRequest>* request_pool, \
        util::ObjectPoolMutiThread<HttpResponse>* response_pool);
    protocol::http::HttpRequest* GetRequest() const;
    protocol::http::HttpResponse* GetResponse() const;
    TcpSslConnection::ptr GetConnection();
    ~HttpsConnectionManager();
private:
    TcpSslConnectionManager m_tcp_ssl_manager;
    HttpProtoStore::ptr m_proto_store;
};
}


#endif