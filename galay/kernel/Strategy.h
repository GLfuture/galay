#ifndef __GALAY_STRATEGY_H__
#define __GALAY_STRATEGY_H__

#include "Base.h"
#include <memory>
#include <string_view>
#include <functional>

namespace galay::coroutine
{
    class Coroutine;
    class CoroutineWaitContext;
    class Awaiter_int;
}

namespace galay::action
{
    class WaitAction;
}

namespace galay::async
{
    class AsyncTcpSocket;
}

namespace galay::scheduler
{
    class EventScheduler;
    class CoroutineScheduler;
}

namespace galay::event
{
class Strategy
{
public:
    using ptr = std::shared_ptr<Strategy>;
    virtual coroutine::Coroutine Execute(async::AsyncTcpSocket* socket \
        , scheduler::EventScheduler* net_scheduler, scheduler::CoroutineScheduler* co_scheduler ) = 0;
};

class UnaryOperation
{
public:
    UnaryOperation(async::AsyncTcpSocket* socket, action::WaitAction* action);
    //should to free the buffer
    std::string_view GetReadBuffer();
    void SetWriteBuffer(std::string_view view, bool on_heap);
    inline void ToBeClosed() { m_close = true; };
    inline bool WillClose() { return m_close; };
    bool WbufferOnHeap();
    virtual ~UnaryOperation() = default;
private:
    bool m_close;
    bool m_wbufferOnHeap;
    action::WaitAction* m_action;
    async::AsyncTcpSocket* m_socket;
};

// recv - business - send
class UnaryStrategy: public Strategy
{
public:
    /*
        scheduler_index :net scheduler's index
    */
    UnaryStrategy(std::function<coroutine::Coroutine(UnaryOperation*,coroutine::CoroutineWaitContext*)> callback);
    virtual coroutine::Coroutine Execute(async::AsyncTcpSocket* socket \
        , scheduler::EventScheduler* net_scheduler, scheduler::CoroutineScheduler* co_scheduler) override;
    virtual ~UnaryStrategy() = default;
private:
    std::function<coroutine::Coroutine(UnaryOperation*,coroutine::CoroutineWaitContext*)> m_user_defined_callback;
};

class ClientStreamOperation{
public:
};

class ClientStreamStrategy: public Strategy
{
public:
    ClientStreamStrategy(std::function<coroutine::Coroutine(ClientStreamOperation*,coroutine::CoroutineWaitContext*)> callback);
    virtual coroutine::Coroutine Execute(async::AsyncTcpSocket* socket \
        , scheduler::EventScheduler* net_scheduler, scheduler::CoroutineScheduler* co_scheduler ) override;
    ~ClientStreamStrategy() = default;
private:
    std::function<coroutine::Coroutine(ClientStreamOperation*,coroutine::CoroutineWaitContext*)> m_user_defined_callback;
};

class ServerStreamOperation{
public:
};

class ServerStreamStrategy: public Strategy
{
public:
};

}
#endif