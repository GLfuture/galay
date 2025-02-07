#ifndef __GALAY_SCHEDULER_H__
#define __GALAY_SCHEDULER_H__

#include <string>
#include <thread>
#include <latch>
#include <functional>
#include <concurrentqueue/moodycamel/blockingconcurrentqueue.h>
#include "galay/common/Base.h"

namespace galay::thread {
    class ThreadWaiters;
}

namespace galay {
    class CoroutineBase;
    class Timer;
}

namespace galay::details {

class Event;
class CallbackEvent;
class EventEngine; 
class AbstractTimeEvent;


class Scheduler
{
public:
    virtual std::string Name() = 0;
    virtual ~Scheduler() = default;
};

class CoroutineScheduler final: public Scheduler
{
public:
    enum class Action
    {
        kActionResume,
        kActionDestroy,
    };
    using ptr = std::shared_ptr<CoroutineScheduler>;
    using uptr = std::unique_ptr<CoroutineScheduler>;
    using Coroutine_wptr = std::weak_ptr<CoroutineBase>; 
    CoroutineScheduler();
    std::string Name() override { return "CoroutineScheduler"; }
    void ToResumeCoroutine(Coroutine_wptr coroutine);
    void ToDestroyCoroutine(Coroutine_wptr coroutine);
    bool Loop(int timeout);
    bool Stop();
    bool IsRunning() const;
private:
    std::atomic_bool m_running;
    std::unique_ptr<std::thread> m_thread;
    std::shared_ptr<thread::ThreadWaiters> m_waiter;
    moodycamel::BlockingConcurrentQueue<std::pair<Action, Coroutine_wptr>> m_coroutines_queue;
};


class EventScheduler: public Scheduler
{
public:
    using ptr = std::shared_ptr<EventScheduler>;
    using uptr = std::unique_ptr<EventScheduler>;

    using timer_ptr = std::shared_ptr<Timer>;
    EventScheduler();
    std::string Name() override { return "EventScheduler"; }
    virtual bool Loop(int timeout);
    virtual bool Stop();
    bool IsRunning() const;
    virtual uint32_t GetErrorCode() const;
    virtual EventEngine* GetEngine() { return m_engine.get(); }
    void AddTimer(timer_ptr timer, int64_t ms);
    ~EventScheduler() = default;
protected:
    std::latch m_latch;
    std::unique_ptr<std::thread> m_thread;
    std::shared_ptr<EventEngine> m_engine;
    std::shared_ptr<AbstractTimeEvent> m_timer_event;
};

class SessionScheduler final: public EventScheduler 
{
public:
    std::string Name() override { return "SessionScheduler"; }
};


}

#endif