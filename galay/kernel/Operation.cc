#include "Operation.h"
#include "Async.h"
#include "Scheduler.h"
#include "Coroutine.h"
#include "Server.h"

namespace galay
{

// StringViewWrapper::StringViewWrapper(std::string_view &origin_view)
//     :m_origin_view(origin_view)
// {
//
// }
//
// std::string_view StringViewWrapper::Data() const
// {
//     return m_origin_view;
// }
//
// void StringViewWrapper::Erase(const int length) const
// {
//     if ( length >= m_origin_view.size() ) return;
//     std::string_view remain = m_origin_view.substr(length);
//     const auto data = new char[remain.size()];
//     memcpy(data, remain.data(), remain.size());
//     if( !m_origin_view.empty() ){
//         delete[] m_origin_view.data();
//         m_origin_view = std::string_view(data, remain.size());
//     }
// }
//
// void StringViewWrapper::Clear() const
// {
//     if( !m_origin_view.empty()) {
//         delete[] m_origin_view.data();
//         m_origin_view = "";
//     }
// }
//
// StringViewWrapper::~StringViewWrapper()
// = default;


TcpConnection::TcpConnection(async::AsyncNetIo* socket)
    :m_socket(socket)
{
}

TcpConnection::~TcpConnection()
{
    delete m_socket;
}

TcpConnectionManager::TcpConnectionManager(std::function<coroutine::Coroutine(TcpConnectionManager)>& callback, async::AsyncNetIo* socket)
    : m_callback(callback), m_connection(std::make_shared<TcpConnection>(socket))
{
}

TcpConnection::ptr TcpConnectionManager::GetConnection()
{
    return m_connection;
}

void TcpConnectionManager::ReExecute(const TcpConnectionManager& operation) const
{
    m_callback(operation);
}


std::any &TcpConnectionManager::GetContext()
{
    return m_context;
}

TcpConnectionManager::~TcpConnectionManager()
= default;

TcpSslConnectionManager::TcpSslConnectionManager(std::function<coroutine::Coroutine(TcpSslConnectionManager)> &callback, async::AsyncSslNetIo* socket)
    : m_callback(callback), m_connection(std::make_shared<TcpSslConnection>(socket))
{
}

TcpSslConnection::ptr TcpSslConnectionManager::GetConnection()
{
    return m_connection;
}

void TcpSslConnectionManager::ReExecute(const TcpSslConnectionManager& operation) const
{
    m_callback(operation);
}


std::any &TcpSslConnectionManager::GetContext()
{
    return m_context;
}

TcpSslConnectionManager::~TcpSslConnectionManager()
= default;

TcpCallbackStore::TcpCallbackStore(const std::function<coroutine::Coroutine(TcpConnectionManager)> &callback)
    :m_callback(callback)
{

}

void TcpCallbackStore::Execute(async::AsyncNetIo* socket)
{
    const TcpConnectionManager operation(m_callback, socket);
    m_callback(operation);
}

TcpSslConnection::TcpSslConnection(async::AsyncSslNetIo* socket)
    :m_socket(socket)
{
}

TcpSslConnection::~TcpSslConnection()
{
    delete m_socket;
}

TcpSslCallbackStore::TcpSslCallbackStore(const std::function<coroutine::Coroutine(TcpSslConnectionManager)> &callback)
    :m_callback(callback)
{
}

void TcpSslCallbackStore::Execute(async::AsyncSslNetIo* socket)
{
    const TcpSslConnectionManager operation(m_callback, socket);
    this->m_callback(operation);
}

HttpOperation::HttpProtoStore::HttpProtoStore(protocol::http::HttpRequest *request, protocol::http::HttpResponse *response)
    :m_request(request), m_response(response)
{
}

HttpOperation::HttpProtoStore::~HttpProtoStore()
{
    // server::HttpServer::ReturnRequest(m_request);
    // server::HttpServer::ReturnResponse(m_response);
}

HttpOperation::HttpOperation(const TcpConnectionManager& operation, protocol::http::HttpRequest *request, protocol::http::HttpResponse *response)
    :m_operation(operation), m_proto_store(std::make_shared<HttpProtoStore>(request, response))
{
}

protocol::http::HttpRequest *HttpOperation::GetRequest() const
{
    return m_proto_store->m_request;
}

protocol::http::HttpResponse *HttpOperation::GetResponse() const
{
    return m_proto_store->m_response;
}

TcpConnection::ptr HttpOperation::GetConnection()
{
    return m_operation.GetConnection();
}

void HttpOperation::Continue() const
{
    m_operation.ReExecute(m_operation);
}

HttpOperation::~HttpOperation()
= default;

}