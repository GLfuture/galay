#include "Strategy.h"
#include "Event.h"
#include "Async.h"
#include "WaitAction.h"
#include "EventEngine.h"
#include "Scheduler.h"
#include <sys/timerfd.h>
#include <spdlog/spdlog.h>

namespace galay::event
{

UnaryOperation::UnaryOperation(async::AsyncTcpSocket *socket, action::WaitAction *action)
    :m_socket(socket), m_action(action), m_wbufferOnHeap(false), m_close(false)
{
}

std::string_view UnaryOperation::GetReadBuffer()
{
    return m_socket->GetRBuffer();
}

void UnaryOperation::SetWriteBuffer(std::string_view view, bool on_heap)
{
    m_socket->SetWBuffer(view);
    m_wbufferOnHeap = on_heap;
}

bool UnaryOperation::WbufferOnHeap()
{
    return m_wbufferOnHeap;
}

UnaryStrategy::UnaryStrategy(std::function<coroutine::Coroutine(UnaryOperation*,coroutine::CoroutineWaitContext*)> callback)
    :m_user_defined_callback(callback)
{
}

coroutine::Coroutine UnaryStrategy::Execute(async::AsyncTcpSocket* socket \
        , scheduler::EventScheduler* net_scheduler, scheduler::CoroutineScheduler* co_scheduler)
{
    RecvEvent recv_event(socket, false);
    SendEvent send_event(socket, false);
    action::WaitAction* action = new action::NetIoEventAction(&recv_event, false, action::kActionToAddEvent);
    coroutine::CoroutineWaiters waiters(0, co_scheduler);
    UnaryOperation operation(socket, action);
    coroutine::CoroutineWaitContext context(waiters);
    //Add timer
    
    //CallbackEvent time_event = new CallbackEvent();
    
    while(1)
    {
        // wait for data
        int ret = co_await socket->Recv(action);
        if( ret < 0 ) {
            spdlog::error("[{}:{}] [[Recv error] [ret: {}, Errmsg:{}]]", __FILE__, __LINE__, ret, socket->GetLastError());
            break;
        } else if ( ret == 0 ) {
            spdlog::info("UnaryStrategy::Execute [Handle: {}] [Close]", socket->GetHandle());
            break;
        }
        m_user_defined_callback(&operation, &context);        
        bool success = co_await waiters.Wait();
        std::string_view wbuffer = socket->GetWBuffer();
        if( !wbuffer.empty()) {
            action->ResetEvent(&send_event, false);
            action->ResetActionType(action::kActionToModEvent);
            socket->SetWBuffer(wbuffer);
            int sendBytes = co_await socket->Send(action);
            if ( operation.WbufferOnHeap() ) {
                delete[] wbuffer.data();
            }
        }
        if( operation.WillClose() ){
            break;
        }
        action->ResetEvent(&recv_event, false);
        action->ResetActionType(action::kActionToModEvent);
    }
    delete action;
    delete socket;
    co_return;
}

ClientStreamStrategy::ClientStreamStrategy(std::function<coroutine::Coroutine(ClientStreamOperation*,coroutine::CoroutineWaitContext*)> callback)
    :m_user_defined_callback(callback)
{
}

coroutine::Coroutine ClientStreamStrategy::Execute(async::AsyncTcpSocket *socket \
        , scheduler::EventScheduler* net_scheduler, scheduler::CoroutineScheduler* co_scheduler)
{
    co_return;
}



}