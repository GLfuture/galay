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
    class AsyncTcpSocket;
};

namespace galay::scheduler
{
    class EventScheduler;
    class CoroutineScheduler;
}

namespace galay::action
{
    class NetIoEventAction;
};

namespace galay
{

#define DEFAULT_CHECK_TIME_MS       5//10 * 1000           //10s
#define DEFAULT_TCP_TIMEOUT_MS      5 * 10//10 * 60 * 1000      //10min

class TcpConnection
{
public:
    TcpConnection(async::AsyncTcpSocket* socket,\
        scheduler::EventScheduler* net_scheduler, scheduler::CoroutineScheduler* co_scheduler);
    coroutine::Awaiter_int WaitForRecv();
    /*
        you should free recv data manually
    */
    std::string_view FetchRecvData();
    /*
        this function will clear temp recv data 
    */
    void ClearRecvData(bool free_memory = false);
    
    void PrepareSendData(std::string_view data);
    coroutine::Awaiter_int WaitForSend();
    

    coroutine::Awaiter_bool CloseConnection();

    scheduler::EventScheduler* GetNetScheduler();
    scheduler::CoroutineScheduler* GetCoScheduler();
    
    ~TcpConnection();
private:
    std::atomic_bool m_event_in_scheduler;
    event::RecvEvent* m_recv_event;
    event::SendEvent* m_send_event;
    action::NetIoEventAction* m_event_action;
    async::AsyncTcpSocket* m_socket;
    scheduler::EventScheduler* m_net_scheduler;
    scheduler::CoroutineScheduler* m_co_scheduler;
};

class TcpOperation
{
public:
    TcpOperation(std::function<coroutine::Coroutine(TcpOperation*)>& callback, async::AsyncTcpSocket* socket, \
        scheduler::EventScheduler* net_scheduler, scheduler::CoroutineScheduler* co_scheduler);
    TcpConnection* GetConnection();

    /*
        ReExecute will flush m_last_active_time, you can also actively call FlushActiveTimer to flush m_last_active_time
    */
    void ReExecute();
    void Done();
    
    void SetTimerCallback(const std::function<void()>& callback);
    void FlushActiveTimer();
    
    ~TcpOperation();
private:
    std::function<coroutine::Coroutine(TcpOperation*)>& m_callback;
    std::any m_context;
    TcpConnection m_connection;
    int64_t m_last_active_time;
    event::TimeEvent::Timer::ptr m_timer;
    std::function<void()> m_timer_callback;
};

class CallbackStore
{
public:
    CallbackStore(const std::function<coroutine::Coroutine(TcpOperation*)>& callback);
    void Execute(async::AsyncTcpSocket* socket, \
        scheduler::EventScheduler* net_scheduler, scheduler::CoroutineScheduler* co_scheduler);
private:
    std::function<coroutine::Coroutine(TcpOperation*)> m_callback;
};


}


#endif