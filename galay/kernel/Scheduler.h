#ifndef __GALAY_SCHEDULER_H__
#define __GALAY_SCHEDULER_H__

#include <string>
#include <thread>
#include <functional>
#include "concurrentqueue/moodycamel/blockingconcurrentqueue.h"
#include "galay/common/Base.h"

namespace galay::coroutine {
    class Coroutine;
};

namespace galay::thread {
    class ThreadWaiters;
}

namespace galay {
    class Timer;
}

namespace galay::details {

class Event;
class CallbackEvent;
class EventEngine; 
class TimeEvent;


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
    using uptr = std::unique_ptr<CoroutineScheduler>;
    CoroutineScheduler();
    std::string Name() override { return "CoroutineScheduler"; }
    void EnqueueCoroutine(coroutine::Coroutine* coroutine);
    bool Loop(int timeout);
    bool Stop();
    bool IsRunning() const;
private:
    std::atomic_bool m_running;
    std::unique_ptr<std::thread> m_thread;
    std::shared_ptr<thread::ThreadWaiters> m_waiter;
    moodycamel::BlockingConcurrentQueue<coroutine::Coroutine*> m_coroutines_queue;
};


class EventScheduler: public Scheduler
{
public:
    using ptr = std::shared_ptr<EventScheduler>;
    using uptr = std::unique_ptr<EventScheduler>;
    EventScheduler();
    std::string Name() override { return "EventScheduler"; }
    virtual bool Loop(int timeout);
    virtual bool Stop() const;
    bool IsRunning() const;
    virtual uint32_t GetErrorCode() const;
    virtual details::EventEngine* GetEngine() { return m_engine.get(); }
    ~EventScheduler() = default;
protected:
    std::unique_ptr<std::thread> m_thread;
    std::shared_ptr<details::EventEngine> m_engine;
    std::shared_ptr<thread::ThreadWaiters> m_waiter;
};

class TimerScheduler final : public EventScheduler
{
public:
    using ptr = std::shared_ptr<TimerScheduler>;
    TimerScheduler();
    std::string Name() override { return "TimerScheduler"; }
    std::shared_ptr<galay::Timer> AddTimer(uint64_t ms, std::function<void(std::shared_ptr<galay::Timer>)>&& callback) const;
    bool Loop(int timeout) override;
    bool Stop() const override;
    bool IsRunning() const;
    uint32_t GetErrorCode() const override;
    details::EventEngine* GetEngine() override { return m_engine.get(); }
    ~TimerScheduler();
private:
    details::TimeEvent* m_timer_event;
};

}

#endif