#ifndef __GALAY_SCHEDULER_H__
#define __GALAY_SCHEDULER_H__

#include <string>
#include <thread>
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
    EventScheduler();
    std::string Name() override { return "EventScheduler"; }
    virtual bool Loop(int timeout);
    virtual bool Stop();
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
    std::shared_ptr<Timer> AddTimer(uint64_t ms, std::function<void(std::weak_ptr<details::TimeEvent>, std::shared_ptr<Timer>)>&& callback) const;
    void AddTimer(const uint64_t ms, const std::shared_ptr<Timer>& timer);
    bool Loop(int timeout) override;
    bool Stop() override;
    bool IsRunning() const;
    uint32_t GetErrorCode() const override;
    details::EventEngine* GetEngine() override { return m_engine.get(); }
    ~TimerScheduler();
private:
    std::shared_ptr<details::TimeEvent> m_timer_event;
};

}

#endif