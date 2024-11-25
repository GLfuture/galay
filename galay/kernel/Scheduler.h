#ifndef __GALAY_SCHEDULER_H__
#define __GALAY_SCHEDULER_H__

#include <string>
#include <memory>
#include <thread>
#include <mutex>
#include <queue>
#include <shared_mutex>
#include <functional>
#include "concurrentqueue/moodycamel/blockingconcurrentqueue.h"
#include "galay/common/Base.h"

namespace galay::event {
    class Event;
    class CallbackEvent;
    class EventEngine; 
    class TimeEvent;
    class Timer;
};

namespace galay::coroutine {
    class Coroutine;
};

namespace galay::thread {
    class ThreadWaiters;
}

namespace galay::scheduler {

class Scheduler
{
public:
    virtual std::string Name() = 0;
};

class CoroutineScheduler: public Scheduler
{
public:
    using ptr = std::shared_ptr<CoroutineScheduler>;
    CoroutineScheduler();
    inline std::string Name() override { return "CoroutineScheduler"; }
    void EnqueueCoroutine(coroutine::Coroutine* coroutine);
    bool Loop();
    bool Stop();
private:
    std::atomic_bool m_start;
    std::unique_ptr<std::thread> m_thread;
    std::shared_ptr<thread::ThreadWaiters> m_waiter;
    moodycamel::BlockingConcurrentQueue<coroutine::Coroutine*> m_coroutines_queue;
};


class EventScheduler: public Scheduler
{
public:
    using ptr = std::shared_ptr<EventScheduler>;
    EventScheduler();
    inline std::string Name() override { return "EventScheduler"; }
    bool Loop(int timeout = -1);
    bool Stop();
    uint32_t GetErrorCode();
    inline event::EventEngine* GetEngine() { return m_engine.get(); }
    virtual ~EventScheduler();
protected:
    std::atomic_bool m_start;
    std::unique_ptr<std::thread> m_thread;
    std::shared_ptr<event::EventEngine> m_engine;
    std::shared_ptr<thread::ThreadWaiters> m_waiter;
};

class TimerScheduler: public EventScheduler
{
public:
    using ptr = std::shared_ptr<TimerScheduler>;
    TimerScheduler();
    inline std::string Name() override { return "TimerScheduler"; }
    std::shared_ptr<event::Timer> AddTimer(int64_t ms, std::function<void(std::shared_ptr<event::Timer>)>&& callback);
    bool Loop(int timeout = -1);
    bool Stop();
    uint32_t GetErrorCode();
    inline event::EventEngine* GetEngine() { return m_engine.get(); }
    virtual ~TimerScheduler();
private:
    event::TimeEvent* m_timer_event;
};

}

#endif