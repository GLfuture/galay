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
    TcpConnection(async::AsyncTcpSocket* socket,\
        scheduler::EventScheduler* net_scheduler, scheduler::CoroutineScheduler* co_scheduler);
    coroutine::Awaiter_int WaitForRecv();
    /*
        you should free recv data manually
    */
    StringViewWrapper FetchRecvData();
    
    void PrepareSendData(std::string_view data);
    coroutine::Awaiter_int WaitForSend();

    coroutine::Awaiter_bool CloseConnection();

    scheduler::EventScheduler* GetNetScheduler();
    scheduler::CoroutineScheduler* GetCoScheduler();
    
    ~TcpConnection();
private:
    event::NetWaitEvent* m_net_event;
    action::NetIoEventAction* m_event_action;
    async::AsyncTcpSocket* m_socket;
    scheduler::EventScheduler* m_net_scheduler;
    scheduler::CoroutineScheduler* m_co_scheduler;
};

class TcpOperation
{
    using Timer = event::TimeEvent::Timer;
public:
    TcpOperation(std::function<coroutine::Coroutine(TcpOperation)>& callback, async::AsyncTcpSocket* socket, \
        scheduler::EventScheduler* net_scheduler, scheduler::CoroutineScheduler* co_scheduler);
    TcpConnection::ptr GetConnection();

    /*
        ReExecute will flush m_last_active_time, you can also actively call FlushActiveTimer to flush m_last_active_time
    */
    void ReExecute(TcpOperation operation);
    
    Timer::ptr AddTimer(int64_t during_ms, const std::function<void()>& timer_callback);

    void SetContext(const std::any& context);
    std::any& GetContext();

    ~TcpOperation();
private:
    std::function<coroutine::Coroutine(TcpOperation)>& m_callback;
    std::any m_context;
    TcpConnection::ptr m_connection;
    event::TimeEvent::Timer::ptr m_timer;
};

class CallbackStore
{
public:
    CallbackStore(const std::function<coroutine::Coroutine(TcpOperation)>& callback);
    void Execute(async::AsyncTcpSocket* socket, \
        scheduler::EventScheduler* net_scheduler, scheduler::CoroutineScheduler* co_scheduler);
private:
    std::function<coroutine::Coroutine(TcpOperation)> m_callback;
};


}


#endif