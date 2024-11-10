#include "Operation.h"
#include "Async.h"
#include "Awaiter.h"
#include "Scheduler.h"
#include "Coroutine.h"
#include "WaitAction.h"
#include "../util/Time.h"
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


TcpConnection::TcpConnection(async::AsyncTcpSocket* socket,\
    scheduler::EventScheduler* net_scheduler, scheduler::CoroutineScheduler* co_scheduler)
    : m_net_scheduler(net_scheduler), m_co_scheduler(co_scheduler)
{
    this->m_net_event = new event::TcpWaitEvent(socket);
    this->m_socket = socket;
    this->m_event_action = new action::TcpEventAction(net_scheduler->GetEngine(), m_net_event);
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

scheduler::EventScheduler *TcpConnection::GetNetScheduler()
{
    return m_net_scheduler;
}

scheduler::CoroutineScheduler *TcpConnection::GetCoScheduler()
{
    return m_co_scheduler;
}

TcpConnection::~TcpConnection()
{
    delete this->m_net_event;
    delete this->m_event_action;
    delete this->m_socket;
}

TcpOperation::TcpOperation(std::function<coroutine::Coroutine(TcpOperation)>& callback, async::AsyncTcpSocket* socket, \
    scheduler::EventScheduler* net_scheduler, scheduler::CoroutineScheduler* co_scheduler)
    : m_callback(callback), m_connection(std::make_shared<TcpConnection>(socket, net_scheduler, co_scheduler)), m_timer(nullptr)
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

TcpOperation::Timer::ptr TcpOperation::AddTimer(int64_t during_ms, const std::function<void()> &timer_callback)
{
    return m_connection->GetNetScheduler()->GetTimeEvent()->AddTimer(during_ms, [this, timer_callback](event::TimeEvent::Timer::ptr timer){
            timer_callback();
        });
};

std::any &TcpOperation::GetContext()
{
    return m_context;
}

TcpOperation::~TcpOperation()
{
}

TcpSslOperation::TcpSslOperation(std::function<coroutine::Coroutine(TcpSslOperation)> &callback, action::TcpSslEventAction *action, scheduler::EventScheduler *net_scheduler, scheduler::CoroutineScheduler *co_scheduler)
    : m_callback(callback), m_timer(nullptr), m_connection(std::make_shared<TcpSslConnection>(action, net_scheduler, co_scheduler))
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

TcpSslOperation::Timer::ptr TcpSslOperation::AddTimer(int64_t during_ms, const std::function<void()> &timer_callback)
{
    return m_connection->GetNetScheduler()->GetTimeEvent()->AddTimer(during_ms, [this, timer_callback](event::TimeEvent::Timer::ptr timer){
            timer_callback();
        });
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

void TcpCallbackStore::Execute(async::AsyncTcpSocket *socket,
                     scheduler::EventScheduler *net_scheduler, scheduler::CoroutineScheduler *co_scheduler)
{
    TcpOperation operaction(m_callback, socket, net_scheduler, co_scheduler);
    m_callback(operaction);
}

TcpSslConnection::TcpSslConnection(action::TcpSslEventAction *action, scheduler::EventScheduler *net_scheduler, scheduler::CoroutineScheduler *co_scheduler)
    : m_net_scheduler(net_scheduler), m_co_scheduler(co_scheduler), m_event_action(action)
{
    this->m_net_event = static_cast<event::TcpSslWaitEvent*>(action->GetBindEvent());
    this->m_socket = m_net_event->GetAsyncTcpSocket();
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

scheduler::EventScheduler *TcpSslConnection::GetNetScheduler()
{
    return m_net_scheduler;
}

scheduler::CoroutineScheduler *TcpSslConnection::GetCoScheduler()
{
    return m_co_scheduler;
}

TcpSslConnection::~TcpSslConnection()
{
    delete m_socket;
    delete m_event_action;
    delete m_net_event;
}

TcpSslCallbackStore::TcpSslCallbackStore(const std::function<coroutine::Coroutine(TcpSslOperation)> &callback)
    :m_callback(callback)
{
}

void TcpSslCallbackStore::Execute(action::TcpSslEventAction *action, scheduler::EventScheduler *net_scheduler, scheduler::CoroutineScheduler *co_scheduler)
{
    TcpSslOperation operation(m_callback, action, net_scheduler, co_scheduler);
    m_callback(operation);
}

}