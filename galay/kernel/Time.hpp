#ifndef GALAY_TIME_HPP
#define GALAY_TIME_HPP

#include <any>
#include <memory>
#include <atomic>
#include <string>
#include <queue>
#include <set>
#include <list>
#include <functional>
#include <shared_mutex>
#include <concurrentqueue/moodycamel/concurrentqueue.h>
#include "Internal.hpp"
#include "galay/utils/System.h"

namespace galay::details {
class TimeEvent;
class TimerManager;
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

    using timer_callback_t = std::function<void(std::weak_ptr<details::TimeEvent>, Timer::ptr)>;
    using timer_manager_ptr = std::weak_ptr<details::TimerManager>;

    static Timer::ptr Create(timer_callback_t &&func);

    Timer(timer_callback_t &&func);
    int64_t GetDeadline() const;
    int64_t GetTimeout() const;
    int64_t GetRemainTime() const;
    std::any& GetUserData() { return m_context; };
    //取消状态的Timer才能调用
    bool ResetTimeout(int64_t timeout);


    void Execute(std::weak_ptr<details::TimeEvent> event);
    void SetIsCancel(bool cancel);
    bool IsCancel() const;

    void SetTimerManager(timer_manager_ptr manager);
    timer_manager_ptr GetTimerManager() const;
private:
    std::any m_context;
    std::atomic_int64_t m_deadline{ -1 };
    std::atomic_bool m_cancel {true};
    timer_callback_t m_callback;
    timer_manager_ptr m_manager;
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
    using wptr = std::weak_ptr<TimerManager>;

    virtual std::list<Timer::ptr> GetArriveredTimers() = 0;
    virtual TimerManagerType Type() = 0;
    virtual Timer::ptr Top() = 0;
    virtual bool IsEmpty()= 0;
    virtual size_t Size() = 0;
    virtual void Push(Timer::ptr timer) = 0;
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
    size_t Size() override { std::shared_lock lock(m_mutex); return m_timers.size(); }
    void Push(Timer::ptr timer) override;
    void UpdateTimers(void* ctx) override;
    int64_t OnceLoopTimeout() override;
private:
    std::shared_mutex m_mutex;
    std::priority_queue<Timer::ptr, std::vector<std::shared_ptr<galay::Timer>>, Timer::TimerCompare> m_timers;  
};

class RbtreeTimerManager: public TimerManager
{
public:
    using ptr = std::shared_ptr<RbtreeTimerManager>;
    std::list<Timer::ptr> GetArriveredTimers() override;
    TimerManagerType Type() override;
    Timer::ptr Top() override;
    size_t Size() override { std::shared_lock lock(m_mutex); return m_timers.size(); }
    bool IsEmpty() override;
    void Push(Timer::ptr timer) override;
    void UpdateTimers(void* ctx) override;
    int64_t OnceLoopTimeout() override;
private:
    std::shared_mutex m_mutex;
    std::set<Timer::ptr, Timer::TimerCompare> m_timers;
};


// class TimeWheelTimerManager: public TimerManager
// {
// public:
//     using ptr = std::shared_ptr<TimeWheelTimerManager>;
//     std::list<Timer::ptr> GetArriveredTimers() override;
//     TimerManagerType Type() override;
//     Timer::ptr Top() override;
//     bool IsEmpty() override;
//     void Push(Timer::ptr timer) override;
//     void UpdateTimers(void* ctx) override;
//     int64_t OnceLoopTimeout() override;
// private:

// };



class TimeEvent: public Event,  public std::enable_shared_from_this<TimeEvent>
{
protected:
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    static TimeEventIDStore g_idStore; 
#endif

    friend class PriorityQueueTimerManager;
    friend class RbtreeTimerManager;
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

    TimerManager::ptr GetTimerManager() { return m_timer_manager; };
    
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