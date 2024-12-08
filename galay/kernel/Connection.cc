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
    : m_connection(std::make_shared<TcpConnection>(socket)), m_userdata(std::make_shared<std::any>())
{
}

TcpConnection::ptr TcpConnectionManager::GetConnection()
{
    return m_connection;
}

std::shared_ptr<std::any> TcpConnectionManager::GetUserData()
{
    return m_userdata;
}

TcpConnectionManager::~TcpConnectionManager()
= default;

TcpSslConnectionManager::TcpSslConnectionManager(async::AsyncSslNetIo* socket)
    : m_connection(std::make_shared<TcpSslConnection>(socket)), m_userdata(std::make_shared<std::any>())
{
}

TcpSslConnection::ptr TcpSslConnectionManager::GetConnection()
{
    return m_connection;
}

std::shared_ptr<std::any> TcpSslConnectionManager::GetUserData()
{
    return m_userdata;
}

TcpSslConnectionManager::~TcpSslConnectionManager()
= default;

TcpCallbackStore::TcpCallbackStore(const std::function<coroutine::Coroutine(TcpConnectionManager)> &callback)
    :m_callback(callback)
{

}

void TcpCallbackStore::Execute(async::AsyncNetIo* socket)
{
    const TcpConnectionManager manager(socket);
    m_callback(manager);
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
    const TcpSslConnectionManager manager(socket);
    m_callback(manager);
}

}