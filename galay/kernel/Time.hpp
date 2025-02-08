#ifndef GALAY_TIME_HPP
#define GALAY_TIME_HPP

#include <any>
#include <memory>
#include <atomic>
#include <string>
#include <queue>
#include <list>
#include <functional>
#include <shared_mutex>
#include <concurrentqueue/moodycamel/concurrentqueue.h>
#include "Internal.hpp"
#include "galay/utils/System.h"

namespace galay::details {
class TimeEvent;
}

namespace galay 
{

enum TimerManagerType
{
    kTimerManagerTypePriorityQueue = 0,
    kTimerManagerTypeRbTree,
    kTimerManagerTypeTimeWheel,
};

/*
    if timer is cancled and callback is not executed, Success while return false
*/
class Timer: public std::enable_shared_from_this<Timer> 
{
    friend class details::TimeEvent;
public:
    using ptr = std::shared_ptr<Timer>;
    using wptr = std::weak_ptr<Timer>;
    class TimerCompare
    {
    public:
        TimerCompare() = default;
        bool operator()(const Timer::ptr &a, const Timer::ptr &b) const;
    };

    using timer_callback = std::function<void(std::weak_ptr<details::TimeEvent>, Timer::ptr)>;

    static Timer::ptr Create(timer_callback &&func);

    Timer(timer_callback &&func);
    int64_t GetTimeout() const;
    int64_t GetDeadline() const;
    int64_t GetRemainTime() const;
    std::any& GetUserData() { return m_context; };
    bool ResetTimeout(int64_t timeout);
    bool ResetTimeout(int64_t timeout, timer_callback &&func);
    void Execute(std::weak_ptr<details::TimeEvent> event);
    bool Start();
    bool Cancle();
    bool IsSuccess() const;
private:
    std::any m_context;
    std::atomic_int64_t m_deadline{ -1 };
    std::atomic_int64_t m_timeout{ -1 };
    std::atomic_bool m_cancle{true};
    std::atomic_bool m_success{false};
    std::function<void(std::weak_ptr<details::TimeEvent>, Timer::ptr)> m_callback;
};

}

namespace galay::details 
{
    

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)

#define DEFAULT_TIMEEVENT_ID_CAPACITY 1024

class TimeEventIDStore
{
public:
    using ptr = std::shared_ptr<TimeEventIDStore>;
    //[0, capacity)
    explicit TimeEventIDStore(int capacity);
    bool GetEventId(int& id);
    bool RecycleEventId(int id);
private:
    int *m_temp;
    int m_capacity;
    moodycamel::ConcurrentQueue<int> m_eventIds;
};
#endif



class TimerManager
{
public:
    using ptr = std::shared_ptr<TimerManager>;

    virtual std::list<Timer::ptr> GetArriveredTimers() = 0;
    virtual TimerManagerType Type() = 0;
    virtual Timer::ptr Top() = 0;
    virtual bool IsEmpty()= 0;
    virtual void Push(Timer::ptr) = 0;
    virtual void UpdateTimers(void* ctx) = 0;
    virtual int64_t OnceLoopTimeout() = 0;
};

class PriorityQueueTimerManager: public TimerManager
{
public:
    using ptr = std::shared_ptr<PriorityQueueTimerManager>;
    std::list<Timer::ptr> GetArriveredTimers() override;
    TimerManagerType Type() override;
    Timer::ptr Top() override;
    bool IsEmpty() override;
    void Push(Timer::ptr timer) override;
    void UpdateTimers(void* ctx) override;
    int64_t OnceLoopTimeout() override;
private:
    std::shared_mutex m_mutex;
    std::priority_queue<Timer::ptr, std::vector<std::shared_ptr<galay::Timer>>, Timer::TimerCompare> m_timers;  
};



class TimeEvent: public Event,  public std::enable_shared_from_this<TimeEvent>
{
protected:
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    static TimeEventIDStore g_idStore; 
#endif

    friend class PriorityQueueTimerManager;
public:
    using ptr = std::shared_ptr<TimeEvent>;
    using wptr = std::weak_ptr<TimeEvent>;
    static bool CreateHandle(GHandle& handle);
    
    TimeEvent(GHandle handle, EventEngine* engine, TimerManagerType type);
    std::string Name() override { return "TimeEvent"; }
    EventType GetEventType() override { return kEventTypeTimer; };

    int64_t OnceLoopTimeout();

    void HandleEvent(EventEngine* engine) override;
    GHandle GetHandle() override { return m_handle; }
    bool SetEventEngine(EventEngine* engine) override;
    EventEngine* BelongEngine() override;

    void AddTimer(const Timer::ptr& timer, const int64_t timeout);
    
    virtual ~TimeEvent();
private:
    void ActiveTimerManager();
private:
    GHandle m_handle;
    std::atomic<EventEngine*> m_engine;
    TimerManager::ptr m_timer_manager;
};


}

#endif