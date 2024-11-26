#ifndef __GALAY_OPERATION_H__
#define __GALAY_OPERATION_H__

#include <any>
#include <atomic>
#include <memory>
#include <string_view>
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
    class NetEventAction;
};

namespace galay::protocol::http
{
    class HttpRequest;
    class HttpResponse;
};

namespace galay
{

#define DEFAULT_CHECK_TIME_MS       10 * 1000           //10s
#define DEFAULT_TCP_TIMEOUT_MS      10 * 60 * 1000      //10min

class StringViewWrapper
{
public:
    StringViewWrapper(std::string_view& origin_view);
    std::string_view Data();
    void Erase(int length);
    void Clear();
    ~StringViewWrapper();
private:
    std::string_view& m_origin_view;
};

class TcpConnection
{
public:
    using ptr = std::shared_ptr<TcpConnection>;
    TcpConnection(async::AsyncNetIo* socket);
    inline async::AsyncNetIo* GetSocket() { return m_socket; }
    
    ~TcpConnection();
private:
    async::AsyncNetIo* m_socket;
};

class TcpOperation
{
    using Timer = event::Timer;
public:
    TcpOperation(std::function<coroutine::Coroutine(TcpOperation)>& callback, async::AsyncNetIo* action);
    TcpConnection::ptr GetConnection();

    /*
        ReExecute will flush m_last_active_time, you can also actively call FlushActiveTimer to flush m_last_active_time
    */
    void ReExecute(TcpOperation operation);
    std::any& GetContext();
    ~TcpOperation();
private:
    std::function<coroutine::Coroutine(TcpOperation)>& m_callback;
    std::any m_context;
    TcpConnection::ptr m_connection;
};

class TcpSslConnection
{
public:
    using ptr = std::shared_ptr<TcpSslConnection>;
    TcpSslConnection(async::AsyncSslNetIo* socket);
    inline async::AsyncSslNetIo* GetSocket() { return m_socket; } 
    ~TcpSslConnection();
private:
    async::AsyncSslNetIo* m_socket;
};

class TcpSslOperation
{
    using Timer = event::Timer;
public:
    TcpSslOperation(std::function<coroutine::Coroutine(TcpSslOperation)>& callback, async::AsyncSslNetIo* socket);
    TcpSslConnection::ptr GetConnection();
    /*
        ReExecute will flush m_last_active_time, you can also actively call FlushActiveTimer to flush m_last_active_time
    */
    void ReExecute(TcpSslOperation operation);
    std::any& GetContext();
    ~TcpSslOperation();
private:
    std::function<coroutine::Coroutine(TcpSslOperation)>& m_callback;
    std::any m_context;
    TcpSslConnection::ptr m_connection;
};

class TcpCallbackStore
{
public:
    TcpCallbackStore(const std::function<coroutine::Coroutine(TcpOperation)>& callback);
    void Execute(async::AsyncNetIo* socket);
private:
    std::function<coroutine::Coroutine(TcpOperation)> m_callback;
};

class TcpSslCallbackStore
{
public:
    TcpSslCallbackStore(const std::function<coroutine::Coroutine(TcpSslOperation)>& callback);
    void Execute(async::AsyncSslNetIo* socket);
private:
    std::function<coroutine::Coroutine(TcpSslOperation)> m_callback;
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
    HttpOperation(TcpOperation operation, protocol::http::HttpRequest* request, protocol::http::HttpResponse* response);
    protocol::http::HttpRequest* GetRequest();
    protocol::http::HttpResponse* GetResponse();
    TcpConnection::ptr GetConnection();
    void Continue();
    ~HttpOperation();
private:
    TcpOperation m_operation;
    HttpProtoStore::ptr m_proto_store;
};

}


#endif