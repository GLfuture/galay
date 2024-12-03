#ifndef GALAY_OPERATION_H
#define GALAY_OPERATION_H

#include <any>
#include <functional>
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
    TcpConnectionManager(std::function<coroutine::Coroutine(TcpConnectionManager)>& callback, async::AsyncNetIo* action);
    TcpConnection::ptr GetConnection();

    /*
        ReExecute will flush m_last_active_time, you can also actively call FlushActiveTimer to flush m_last_active_time
    */
    void ReExecute(const TcpConnectionManager& operation) const;
    std::any& GetContext();
    ~TcpConnectionManager();
private:
    std::any m_context;
    TcpConnection::ptr m_connection;
    std::function<coroutine::Coroutine(TcpConnectionManager)>& m_callback;
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
    TcpSslConnectionManager(std::function<coroutine::Coroutine(TcpSslConnectionManager)>& callback, async::AsyncSslNetIo* socket);
    TcpSslConnection::ptr GetConnection();
    /*
        ReExecute will flush m_last_active_time, you can also actively call FlushActiveTimer to flush m_last_active_time
    */
    void ReExecute(const TcpSslConnectionManager& operation) const;
    std::any& GetContext();
    ~TcpSslConnectionManager();
private:
    std::function<coroutine::Coroutine(TcpSslConnectionManager)>& m_callback;
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

class HttpOperation
{
    struct HttpProtoStore
    {
        using ptr = std::shared_ptr<HttpProtoStore>;
        HttpProtoStore(protocol::http::HttpRequest* request, protocol::http::HttpResponse* response);
        protocol::http::HttpRequest* m_request;
        protocol::http::HttpResponse* m_response;
        ~HttpProtoStore();
    };
public:
    HttpOperation(const TcpConnectionManager& operation, protocol::http::HttpRequest* request, protocol::http::HttpResponse* response);
    protocol::http::HttpRequest* GetRequest() const;
    protocol::http::HttpResponse* GetResponse() const;
    TcpConnection::ptr GetConnection();
    void Continue() const;
    ~HttpOperation();
private:
    TcpConnectionManager m_operation;
    HttpProtoStore::ptr m_proto_store;
};

}


#endif