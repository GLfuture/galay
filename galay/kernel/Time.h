#ifndef GALAY_TIME_H
#define GALAY_TIME_H

#include <any>
#include <memory>
#include <atomic>
#include <string>
#include <queue>
#include <functional>
#include <shared_mutex>
#include "Internal.hpp"

namespace galay::details {
class TimeEvent;
}

namespace galay {

extern int64_t GetCurrentTimeMs(); 
extern std::string GetCurrentGMTTimeString();

/*
    if timer is cancled and callback is not executed, Success while return false
*/
class Timer: public std::enable_shared_from_this<Timer> 
{
    friend class details::TimeEvent;
public:
    using ptr = std::shared_ptr<Timer>;
    class TimerCompare
    {
    public:
        TimerCompare() = default;
        bool operator()(const Timer::ptr &a, const Timer::ptr &b) const;
    };
    Timer(uint64_t timeout, std::function<void(std::weak_ptr<details::TimeEvent>, Timer::ptr)> &&func);
    uint64_t GetTimeout() const;
    uint64_t GetDeadline() const;
    uint64_t GetRemainTime() const;
    std::any& GetUserData() { return m_context; };
    bool ResetTimeout(uint64_t timeout);
    bool ResetTimeout(uint64_t timeout, std::function<void(std::weak_ptr<details::TimeEvent>, Timer::ptr)> &&func);
    void Execute(std::weak_ptr<details::TimeEvent> event);
    bool Cancle();
    bool IsSuccess() const;
private:
    std::any m_context;
    std::atomic_uint64_t m_deadline{};
    std::atomic_uint64_t m_timeout{};
    std::atomic_bool m_cancle{false};
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

class TimeEvent final: public Event,  public std::enable_shared_from_this<TimeEvent>
{
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    static TimeEventIDStore g_idStore; 
#endif
public:
    using ptr = std::shared_ptr<TimeEvent>;
    using wptr = std::weak_ptr<TimeEvent>;
    static bool CreateHandle(GHandle& handle);
    
    TimeEvent(GHandle handle, EventEngine* engine);
    std::string Name() override { return "TimeEvent"; };
    void HandleEvent(EventEngine* engine) override;
    EventType GetEventType() override { return kEventTypeTimer; };
    GHandle GetHandle() override { return m_handle; }
    bool SetEventEngine(EventEngine* engine) override;
    EventEngine* BelongEngine() override;
    Timer::ptr AddTimer(uint64_t during_time, std::function<void(std::weak_ptr<details::TimeEvent>, Timer::ptr)> &&func); // ms
    void AddTimer(const uint64_t timeout, const Timer::ptr& timer);
    ~TimeEvent() override;
private:
#ifdef __linux__
    void UpdateTimers();
#endif
private:
    GHandle m_handle;
    std::atomic<EventEngine*> m_engine;
    std::shared_mutex m_mutex;
    std::priority_queue<Timer::ptr, std::vector<std::shared_ptr<galay::Timer>>, Timer::TimerCompare> m_timers;
};


}

#endif