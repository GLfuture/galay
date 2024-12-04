#include "Connection.h"
#include "Async.h"
#include "Scheduler.h"
#include "Coroutine.h"
#include "Server.h"

namespace galay
{
TcpConnection::TcpConnection(async::AsyncNetIo* socket)
    :m_socket(socket)
{
}

TcpConnection::~TcpConnection()
{
    delete m_socket;
}

TcpConnectionManager::TcpConnectionManager(async::AsyncNetIo* socket)
    : m_connection(std::make_shared<TcpConnection>(socket)), m_context(std::make_shared<std::any>())
{
}

TcpConnection::ptr TcpConnectionManager::GetConnection()
{
    return m_connection;
}

std::shared_ptr<std::any> TcpConnectionManager::GetContext()
{
    return m_context;
}

TcpConnectionManager::~TcpConnectionManager()
= default;

TcpSslConnectionManager::TcpSslConnectionManager(async::AsyncSslNetIo* socket)
    : m_connection(std::make_shared<TcpSslConnection>(socket)), m_context(std::make_shared<std::any>())
{
}

TcpSslConnection::ptr TcpSslConnectionManager::GetConnection()
{
    return m_connection;
}

std::shared_ptr<std::any> TcpSslConnectionManager::GetContext()
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
    const TcpConnectionManager operation(socket);
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
    const TcpSslConnectionManager operation(socket);
    this->m_callback(operation);
}

HttpProtoStore::HttpProtoStore(util::ObjectPoolMutiThread<HttpRequest>* request_pool, \
    util::ObjectPoolMutiThread<HttpResponse>* response_pool)
    :m_request(request_pool->GetObjector()), m_response(response_pool->GetObjector()), m_request_pool(request_pool), m_response_pool(response_pool)
{
}

HttpProtoStore::~HttpProtoStore()
{
    m_request_pool->ReturnObjector(m_request);
    m_response_pool->ReturnObjector(m_response);
}

HttpConnectionManager::HttpConnectionManager(const TcpConnectionManager& manager, util::ObjectPoolMutiThread<HttpRequest>* request_pool, \
    util::ObjectPoolMutiThread<HttpResponse>* response_pool)
    :m_tcp_manager(manager), m_proto_store(std::make_shared<HttpProtoStore>(request_pool, response_pool))
{
}

protocol::http::HttpRequest *HttpConnectionManager::GetRequest() const
{
    return m_proto_store->m_request;
}

protocol::http::HttpResponse *HttpConnectionManager::GetResponse() const
{
    return m_proto_store->m_response;
}

TcpConnection::ptr HttpConnectionManager::GetConnection()
{
    return m_tcp_manager.GetConnection();
}

HttpConnectionManager::~HttpConnectionManager()
= default;

HttpsConnectionManager::HttpsConnectionManager(const TcpSslConnectionManager &manager, util::ObjectPoolMutiThread<HttpRequest> *request_pool, util::ObjectPoolMutiThread<HttpResponse> *response_pool)
    :m_tcp_ssl_manager(manager), m_proto_store(std::make_shared<HttpProtoStore>(request_pool, response_pool))
{
}

protocol::http::HttpRequest *HttpsConnectionManager::GetRequest() const
{
    return m_proto_store->m_request;
}

protocol::http::HttpResponse *HttpsConnectionManager::GetResponse() const
{
    return m_proto_store->m_response;
}

TcpSslConnection::ptr HttpsConnectionManager::GetConnection()
{
    return m_tcp_ssl_manager.GetConnection();
}

HttpsConnectionManager::~HttpsConnectionManager() 
= default;
}