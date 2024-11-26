#include "Operation.h"
#include "Async.h"
#include "Awaiter.h"
#include "Scheduler.h"
#include "Coroutine.h"
#include "WaitAction.h"
#include "galay/util/Time.h"
#include "Server.h"
#include <cstring>

namespace galay
{

StringViewWrapper::StringViewWrapper(std::string_view &origin_view)
    :m_origin_view(origin_view)
{
    
}

std::string_view StringViewWrapper::Data()
{
    return m_origin_view;
}

void StringViewWrapper::Erase(int length)
{
    if ( length >= m_origin_view.size() ) return;
    std::string_view remain = m_origin_view.substr(length);
    char* data = new char[remain.size()];
    memcpy(data, remain.data(), remain.size());
    if( !m_origin_view.empty() ){
        delete[] m_origin_view.data();
        m_origin_view = std::string_view(data, remain.size());
    }
}

void StringViewWrapper::Clear()
{
    if( !m_origin_view.empty()) {
        delete[] m_origin_view.data();
        m_origin_view = "";
    }
}

StringViewWrapper::~StringViewWrapper()
{
}


TcpConnection::TcpConnection(async::AsyncNetIo* socket)
    :m_socket(socket)
{
}

TcpConnection::~TcpConnection()
{
    delete m_socket;
}

TcpOperation::TcpOperation(std::function<coroutine::Coroutine(TcpOperation)>& callback, async::AsyncNetIo* socket)
    : m_callback(callback), m_connection(std::make_shared<TcpConnection>(socket))
{
}

TcpConnection::ptr TcpOperation::GetConnection()
{
    return m_connection;
}

void TcpOperation::ReExecute(TcpOperation operation)
{
    m_callback(operation);
}


std::any &TcpOperation::GetContext()
{
    return m_context;
}

TcpOperation::~TcpOperation()
{
}

TcpSslOperation::TcpSslOperation(std::function<coroutine::Coroutine(TcpSslOperation)> &callback, async::AsyncSslNetIo* socket)
    : m_callback(callback), m_connection(std::make_shared<TcpSslConnection>(socket))
{
}

TcpSslConnection::ptr TcpSslOperation::GetConnection()
{
    return m_connection;
}

void TcpSslOperation::ReExecute(TcpSslOperation operation)
{
    m_callback(operation);
}


std::any &TcpSslOperation::GetContext()
{
    return m_context;
}

TcpSslOperation::~TcpSslOperation()
{
}

TcpCallbackStore::TcpCallbackStore(const std::function<coroutine::Coroutine(TcpOperation)> &callback)
    :m_callback(callback)
{

}

void TcpCallbackStore::Execute(async::AsyncNetIo* socket)
{
    TcpOperation operaction(m_callback, socket);
    m_callback(operaction);
}

TcpSslConnection::TcpSslConnection(async::AsyncSslNetIo* socket)
    :m_socket(socket)
{
}

TcpSslConnection::~TcpSslConnection()
{
    delete m_socket;
}

TcpSslCallbackStore::TcpSslCallbackStore(const std::function<coroutine::Coroutine(TcpSslOperation)> &callback)
    :m_callback(callback)
{
}

void TcpSslCallbackStore::Execute(async::AsyncSslNetIo* socket)
{
    TcpSslOperation operation(m_callback, socket);
    m_callback(operation);
}

HttpOperation::HttpProtoStore::HttpProtoStore(protocol::http::HttpRequest *request, protocol::http::HttpResponse *response)
    :m_request(request), m_response(response)
{
}

HttpOperation::HttpProtoStore::~HttpProtoStore()
{
    server::HttpServer::ReturnRequest(m_request);
    server::HttpServer::ReturnResponse(m_response);
}

HttpOperation::HttpOperation(TcpOperation operation, protocol::http::HttpRequest *request, protocol::http::HttpResponse *response)
    :m_operation(operation), m_proto_store(std::make_shared<HttpProtoStore>(request, response))
{
}

protocol::http::HttpRequest *HttpOperation::GetRequest()
{
    return m_proto_store->m_request;
}

protocol::http::HttpResponse *HttpOperation::GetResponse()
{
    return m_proto_store->m_response;
}

TcpConnection::ptr HttpOperation::GetConnection()
{
    return m_operation.GetConnection();
}

void HttpOperation::Continue()
{
    m_operation.ReExecute(m_operation);
}

HttpOperation::~HttpOperation()
{
}

}