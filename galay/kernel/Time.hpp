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
class AbstractTimeEvent;
}

namespace galay 
{

/*
    if timer is cancled and callback is not executed, Success while return false
*/
class Timer: public std::enable_shared_from_this<Timer> 
{
    friend class details::AbstractTimeEvent;
public:
    using ptr = std::shared_ptr<Timer>;
    using wptr = std::weak_ptr<Timer>;
    class TimerCompare
    {
    public:
        TimerCompare() = default;
        bool operator()(const Timer::ptr &a, const Timer::ptr &b) const;
    };

    using timer_callback = std::function<void(std::weak_ptr<details::AbstractTimeEvent>, Timer::ptr)>;

    static Timer::ptr Create(timer_callback &&func);

    Timer(timer_callback &&func);
    int64_t GetTimeout() const;
    int64_t GetDeadline() const;
    int64_t GetRemainTime() const;
    std::any& GetUserData() { return m_context; };
    bool ResetTimeout(int64_t timeout);
    bool ResetTimeout(int64_t timeout, timer_callback &&func);
    void Execute(std::weak_ptr<details::AbstractTimeEvent> event);
    bool Start();
    bool Cancle();
    bool IsSuccess() const;
private:
    std::any m_context;
    std::atomic_int64_t m_deadline{ -1 };
    std::atomic_int64_t m_timeout{ -1 };
    std::atomic_bool m_cancle{true};
    std::atomic_bool m_success{false};
    std::function<void(std::weak_ptr<details::AbstractTimeEvent>, Timer::ptr)> m_callback;
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

enum class TimerManagerType: uint8_t
{
    kPriorityQueueTimerManager = 0x01,
    kRBTreeTimerManager = 0x02
};

template<TimerManagerType Type>
class TimerManager
{
public:
    using ptr = std::shared_ptr<TimerManager>;
    std::list<Timer::ptr> GetArriveredTimers() { return std::list<Timer::ptr>(); }
    Timer::ptr Top() { return nullptr; }
    bool IsEmpty() { return true; }
    void Push(Timer::ptr) {}
};

template<>
class TimerManager<TimerManagerType::kPriorityQueueTimerManager>
{
public:
    using ptr = std::shared_ptr<TimerManager>;
    std::list<Timer::ptr> GetArriveredTimers();
    Timer::ptr Top();
    bool IsEmpty();
    void Push(Timer::ptr timer);
private:
    std::shared_mutex m_mutex;
    std::priority_queue<Timer::ptr, std::vector<std::shared_ptr<galay::Timer>>, Timer::TimerCompare> m_timers;  
};

template<TimerManagerType Type>
void UpdateTimers(AbstractTimeEvent* event, TimerManager<Type>& manager);

class AbstractTimeEvent: public Event,  public std::enable_shared_from_this<AbstractTimeEvent>
{
protected:
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    static TimeEventIDStore g_idStore; 
#endif
    template<TimerManagerType Type>
    friend void UpdateTimers(AbstractTimeEvent* event, TimerManager<Type>& manager);
public:
    using ptr = std::shared_ptr<AbstractTimeEvent>;
    using wptr = std::weak_ptr<AbstractTimeEvent>;
    static bool CreateHandle(GHandle& handle);
    
    AbstractTimeEvent(GHandle handle, EventEngine* engine);
    EventType GetEventType() override { return kEventTypeTimer; };
    GHandle GetHandle() override { return m_handle; }
    bool SetEventEngine(EventEngine* engine) override;
    EventEngine* BelongEngine() override;

    virtual void AddTimer(const Timer::ptr& timer, const int64_t timeout) = 0;
    
    virtual ~AbstractTimeEvent() = default;

protected:
    GHandle m_handle;
    std::atomic<EventEngine*> m_engine;
};

template<TimerManagerType Type>
class TimeEvent: public AbstractTimeEvent
{
public:
    static bool CreateHandle(GHandle& handle);

    TimeEvent(GHandle handle, EventEngine* engine);
    std::string Name() override;
    void HandleEvent(EventEngine* engine) override;
    void AddTimer(const Timer::ptr& timer, const int64_t timeout) override;
    virtual ~TimeEvent();
private:
    TimerManager<Type> m_timer_manager;
};



}

#include "Time.tcc"

#endif