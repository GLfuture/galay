#ifndef __GALAY_SCHEDULER_H__
#define __GALAY_SCHEDULER_H__

#include <string>
#include <thread>
#include <functional>
#include "concurrentqueue/moodycamel/blockingconcurrentqueue.h"
#include "galay/common/Base.h"

namespace galay::event {
    class Event;
    class CallbackEvent;
    class EventEngine; 
    class TimeEvent;
};

namespace galay::coroutine {
    class Coroutine;
};

namespace galay::thread {
    class ThreadWaiters;
}

namespace galay {
    class Timer;
}

namespace galay::scheduler {

class Scheduler
{
public:
    virtual std::string Name() = 0;
    virtual ~Scheduler() = default;
};

class CoroutineScheduler final : public Scheduler
{
public:
    using ptr = std::shared_ptr<CoroutineScheduler>;
    CoroutineScheduler();
    std::string Name() override { return "CoroutineScheduler"; }
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
    std::string Name() override { return "EventScheduler"; }
    virtual bool Loop(int timeout);
    virtual bool Stop() const;
    virtual uint32_t GetErrorCode() const;
    virtual event::EventEngine* GetEngine() { return m_engine.get(); }
    ~EventScheduler() override;
protected:
    std::atomic_bool m_start;
    std::unique_ptr<std::thread> m_thread;
    std::shared_ptr<event::EventEngine> m_engine;
    std::shared_ptr<thread::ThreadWaiters> m_waiter;
};

class TimerScheduler final : public EventScheduler
{
public:
    using ptr = std::shared_ptr<TimerScheduler>;
    TimerScheduler();
    std::string Name() override { return "TimerScheduler"; }
    std::shared_ptr<galay::Timer> AddTimer(int64_t ms, std::function<void(std::shared_ptr<galay::Timer>)>&& callback) const;
    bool Loop(int timeout) override;
    bool Stop() const override;
    uint32_t GetErrorCode() const override;
    event::EventEngine* GetEngine() override { return m_engine.get(); }
    ~TimerScheduler() override;
private:
    event::TimeEvent* m_timer_event;
};

}

#endif