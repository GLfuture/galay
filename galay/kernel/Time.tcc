#ifndef GALAY_TIME_TCC
#define GALAY_TIME_TCC

#include "Time.hpp"

namespace galay::details
{


inline std::list<Timer::ptr> TimerManager<TimerManagerType::kPriorityQueueTimerManager>::GetArriveredTimers()
{
    std::list<Timer::ptr> timers;
    int64_t now = utils::GetCurrentTimeMs();
    std::unique_lock lock(this->m_mutex);
    while (!m_timers.empty() && m_timers.top()->GetDeadline() <= now) {
        auto timer = m_timers.top();
        m_timers.pop();
        timers.emplace_back(timer);
    }
    return timers;
}

inline Timer::ptr TimerManager<TimerManagerType::kPriorityQueueTimerManager>::Top()
{
    std::shared_lock lock(this->m_mutex);
    if (m_timers.empty())
    {
        return nullptr;
    }
    return m_timers.top();
}

inline bool TimerManager<TimerManagerType::kPriorityQueueTimerManager>::IsEmpty()
{
    std::shared_lock lock(m_mutex);
    return m_timers.empty();
}

inline void TimerManager<TimerManagerType::kPriorityQueueTimerManager>::Push(Timer::ptr timer)
{
    std::unique_lock lock(this->m_mutex);
    m_timers.push(timer);
}


template<TimerManagerType Type>
inline void UpdateTimers(AbstractTimeEvent* event, TimerManager<Type>& manager)
{
#ifdef __linux__
    struct timespec abstime;
    if (manager.IsEmpty())
    {
        abstime.tv_sec = 0;
        abstime.tv_nsec = 0;
    }
    else
    {
        auto timer = manager.Top();
        int64_t time = timer->GetRemainTime();
        if (time != 0)
        {
            abstime.tv_sec = time / 1000;
            abstime.tv_nsec = (time % 1000) * 1000000;
        }
        else
        {
            abstime.tv_sec = 0;
            abstime.tv_nsec = 1;
        }
    }
    struct itimerspec its = {
        .it_interval = {},
        .it_value = abstime};
    timerfd_settime(event->m_handle.fd, 0, &its, nullptr);
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    if(manager.IsEmpty()) {
        return;
    } else {
        auto timer = manager.Top();
        event->m_engine.load()->ModEvent(event, timer.get());
    }
#endif

}

template <TimerManagerType Type>
inline bool TimeEvent<Type>::CreateHandle(GHandle &handle)
{
    return AbstractTimeEvent::CreateHandle(handle);
}

template <TimerManagerType Type>
inline TimeEvent<Type>::TimeEvent(GHandle handle, EventEngine *engine)
    : AbstractTimeEvent(handle, engine)
{
#if defined(__linux__)
    engine->AddEvent(this, nullptr);
#endif
}

template <TimerManagerType Type>
inline std::string TimeEvent<Type>::Name()
{
    return "TimeEvent";
}

template <TimerManagerType Type>
inline void TimeEvent<Type>::HandleEvent(EventEngine *engine)
{
    std::list<Timer::ptr> timers = m_timer_manager.GetArriveredTimers();
    for (auto timer: timers)
    {
        timer->Execute(weak_from_this());
    }
    UpdateTimers<Type>(this, m_timer_manager);
}

template <TimerManagerType Type>
inline void TimeEvent<Type>::AddTimer(const Timer::ptr &timer, const int64_t timeout)
{
    timer->ResetTimeout(timeout);
    timer->Start();
    this->m_timer_manager.Push(timer);
    UpdateTimers<Type>(this, m_timer_manager);
}

template <TimerManagerType Type>
inline TimeEvent<Type>::~TimeEvent()
{
    m_engine.load()->DelEvent(this, nullptr);
#if defined(__linux__)
    close(m_handle.fd);
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    g_idStore.RecycleEventId(m_handle.fd);
#endif
}
}


#endif