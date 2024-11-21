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


TcpConnection::TcpConnection(action::TcpEventAction* action)
    :m_event_action(action)
{
    m_socket = static_cast<event::TcpWaitEvent*>(action->GetBindEvent())->GetAsyncTcpSocket();
}

coroutine::Awaiter_int TcpConnection::WaitForRecv()
{
    return m_socket->Recv(m_event_action);
}

StringViewWrapper TcpConnection::FetchRecvData()
{
    return StringViewWrapper(m_socket->GetRBuffer());
}

void TcpConnection::PrepareSendData(std::string_view data)
{
    m_socket->SetWBuffer(data);
}

coroutine::Awaiter_int TcpConnection::WaitForSend()
{
    return m_socket->Send(m_event_action);
}

coroutine::Awaiter_bool TcpConnection::CloseConnection()
{
    return m_socket->Close(m_event_action);
}

TcpConnection::~TcpConnection()
{
    delete m_event_action;
}

TcpOperation::TcpOperation(std::function<coroutine::Coroutine(TcpOperation)>& callback,  action::TcpEventAction* action)
    : m_callback(callback), m_connection(std::make_shared<TcpConnection>(action))
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

TcpSslOperation::TcpSslOperation(std::function<coroutine::Coroutine(TcpSslOperation)> &callback, action::TcpSslEventAction *action)
    : m_callback(callback), m_connection(std::make_shared<TcpSslConnection>(action))
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

void TcpCallbackStore::Execute(action::TcpEventAction* action)
{
    TcpOperation operaction(m_callback, action);
    m_callback(operaction);
}

TcpSslConnection::TcpSslConnection(action::TcpSslEventAction *action)
    :m_event_action(action)
{
    this->m_socket = static_cast<event::TcpSslWaitEvent*>(action->GetBindEvent())->GetAsyncTcpSocket();
}

coroutine::Awaiter_int TcpSslConnection::WaitForSslRecv()
{
    return m_socket->SSLRecv(m_event_action);
}

StringViewWrapper TcpSslConnection::FetchRecvData()
{
    return StringViewWrapper(m_socket->GetRBuffer());
}

void TcpSslConnection::PrepareSendData(std::string_view data)
{
    m_socket->SetWBuffer(data);
}

coroutine::Awaiter_int TcpSslConnection::WaitForSslSend()
{
    return m_socket->SSLSend(m_event_action);
}

coroutine::Awaiter_bool TcpSslConnection::CloseConnection()
{
    return m_socket->SSLClose(m_event_action);
}

TcpSslConnection::~TcpSslConnection()
{
    delete m_event_action;
}

TcpSslCallbackStore::TcpSslCallbackStore(const std::function<coroutine::Coroutine(TcpSslOperation)> &callback)
    :m_callback(callback)
{
}

void TcpSslCallbackStore::Execute(action::TcpSslEventAction *action)
{
    TcpSslOperation operation(m_callback, action);
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

coroutine::Awaiter_int HttpOperation::ReturnResponse(std::string response)
{
    m_operation.GetConnection()->PrepareSendData(response);
    return m_operation.GetConnection()->WaitForSend();
}

coroutine::Awaiter_bool HttpOperation::CloseConnection()
{
    return m_operation.GetConnection()->CloseConnection();
}

void HttpOperation::Continue()
{
    m_operation.ReExecute(m_operation);
}

HttpOperation::~HttpOperation()
{
}

}