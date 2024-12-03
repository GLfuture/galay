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

#define DEFAULT_CHECK_TIME_MS       (10 * 1000)           //10s
#define DEFAULT_TCP_TIMEOUT_MS      (10 * 60 * 1000)      //10min

// class StringViewWrapper
// {
// public:
//     explicit StringViewWrapper(std::string_view& origin_view);
//     std::string_view Data() const;
//     void Erase(int length) const;
//     void Clear() const;
//     ~StringViewWrapper();
// private:
//     std::string_view& m_origin_view;
// };

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
    using Timer = event::Timer;
public:
    TcpConnectionManager(async::AsyncNetIo* action);
    TcpConnection::ptr GetConnection();
    std::any& GetContext();
    ~TcpConnectionManager();
private:
    std::any m_context;
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
    using Timer = event::Timer;
public:
    TcpSslConnectionManager(async::AsyncSslNetIo* socket);
    TcpSslConnection::ptr GetConnection();
    std::any& GetContext();
    ~TcpSslConnectionManager();
private:
    std::any m_context;
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

class HttpConnectionManager
{
    using HttpRequest = protocol::http::HttpRequest;
    using HttpResponse = protocol::http::HttpResponse;

    struct HttpProtoStore
    {
        using ptr = std::shared_ptr<HttpProtoStore>;
        HttpProtoStore(util::ObjectPoolMutiThread<HttpRequest>* request_pool, \
            util::ObjectPoolMutiThread<HttpResponse>* response_pool);
        protocol::http::HttpRequest* m_request;
        protocol::http::HttpResponse* m_response;
        util::ObjectPoolMutiThread<HttpRequest>* m_request_pool;
        util::ObjectPoolMutiThread<HttpResponse>* m_response_pool;
        ~HttpProtoStore();
    };
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

}


#endif