#include "Operation.h"
#include "Async.h"
#include "Awaiter.h"
#include "Scheduler.h"
#include "Coroutine.h"
#include "WaitAction.h"
#include "../util/Time.h"

namespace galay
{
TcpConnection::TcpConnection(async::AsyncTcpSocket* socket,\
    scheduler::EventScheduler* net_scheduler, scheduler::CoroutineScheduler* co_scheduler)
    : m_net_scheduler(net_scheduler), m_co_scheduler(co_scheduler)
{
    this->m_recv_event = new event::RecvEvent(net_scheduler->GetEngine(), socket);
    this->m_send_event = new event::SendEvent(net_scheduler->GetEngine(), socket);
    this->m_socket = socket;
    this->m_event_action = new action::NetIoEventAction();
    this->m_event_in_scheduler = false;
}

coroutine::Awaiter_int TcpConnection::WaitForRecv()
{
    m_event_action->ResetEvent(m_recv_event);
    if( ! m_event_in_scheduler.load() ) {
        m_event_action->ResetActionType(action::NetIoEventAction::kActionToAddEvent);
        this->m_event_in_scheduler = true;
    }else{
        m_event_action->ResetActionType(action::NetIoEventAction::kActionToModEvent);
    }
    return m_socket->Recv(m_event_action);
}

std::string_view TcpConnection::FetchRecvData()
{
    return m_socket->GetRBuffer();
}

void TcpConnection::ClearRecvData(bool free_memory)
{
    if( free_memory ){
        std::string_view data = m_socket->GetRBuffer();
        delete[] data.data();
    }
    m_socket->ClearRBuffer();
}

void TcpConnection::PrepareSendData(std::string_view data)
{
    m_socket->SetWBuffer(data);
}

coroutine::Awaiter_int TcpConnection::WaitForSend()
{
    m_event_action->ResetEvent(m_send_event);
    if( !m_event_in_scheduler.load() ) {
        m_event_action->ResetActionType(action::NetIoEventAction::kActionToAddEvent);
        this->m_event_in_scheduler = true;
    }else{
        m_event_action->ResetActionType(action::NetIoEventAction::kActionToModEvent);
    }
    return m_socket->Send(m_event_action);
}

coroutine::Awaiter_bool TcpConnection::CloseConnection()
{
    if( m_event_in_scheduler.load()) {
        m_event_in_scheduler.store(false);
        m_net_scheduler->DelEvent(m_recv_event);
    }
    return m_socket->Close();
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
    delete this->m_recv_event;
    delete this->m_send_event;
    delete this->m_event_action;
    delete this->m_socket;
}

TcpOperation::TcpOperation(std::function<coroutine::Coroutine(TcpOperation*)>& callback, async::AsyncTcpSocket* socket, \
    scheduler::EventScheduler* net_scheduler, scheduler::CoroutineScheduler* co_scheduler)
    : m_callback(callback), m_connection(socket, net_scheduler, co_scheduler), m_timer(nullptr)
{
}

TcpConnection *TcpOperation::GetConnection()
{
    return &m_connection;
}

void TcpOperation::ReExecute()
{
    m_last_active_time = time::GetCurrentTime();
    m_callback(this);
}

void TcpOperation::Done()
{
    this->m_timer->Cancle();
    delete this;
}

void TcpOperation::SetTimerCallback(const std::function<void()> &callback)
{
    m_timer_callback = callback;
}

void TcpOperation::FlushActiveTimer()
{
    m_last_active_time = time::GetCurrentTime();
    if( !m_timer )
    {
        this->m_timer = m_connection.GetNetScheduler()->GetTimeEvent()->AddTimer(DEFAULT_CHECK_TIME_MS, [this](event::TimeEvent::Timer::ptr timer){
            if( time::GetCurrentTime() >= m_last_active_time + DEFAULT_TCP_TIMEOUT_MS) {
                if(m_timer_callback) m_timer_callback();
            } else {
                m_connection.GetNetScheduler()->GetTimeEvent()->ReAddTimer(DEFAULT_CHECK_TIME_MS, timer);
            }
        });
    }
}

TcpOperation::~TcpOperation()
{
}

CallbackStore::CallbackStore(const std::function<coroutine::Coroutine(TcpOperation *)> &callback)
    :m_callback(callback)
{

}

void CallbackStore::Execute(async::AsyncTcpSocket *socket,
                     scheduler::EventScheduler *net_scheduler, scheduler::CoroutineScheduler *co_scheduler)
{
    TcpOperation* operaction = new TcpOperation(m_callback, socket, net_scheduler, co_scheduler);
    m_callback(operaction);
}



}