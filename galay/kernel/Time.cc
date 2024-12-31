#include "Time.h"
#include "EventEngine.h"
#include "galay/utils/System.h"
#if defined(__linux__)
#include <sys/timerfd.h>
#elif  defined(WIN32) || defined(_WIN32) || defined(_WIN32_) || defined(WIN64) || defined(_WIN64) || defined(_WIN64_)

#endif



namespace galay
{

Timer::Timer(const uint64_t timeout, std::function<void(std::weak_ptr<details::TimeEvent>, Timer::ptr)> &&func)
{
    m_callback = std::move(func);
    ResetTimeout(timeout);
}

uint64_t
Timer::GetTimeout() const
{
    return m_timeout;
}

uint64_t
Timer::GetDeadline() const
{
    return m_deadline;
}

uint64_t
Timer::GetRemainTime() const
{
    const int64_t time = m_deadline - utils::GetCurrentTimeMs();
    return time < 0 ? 0 : time;
}

bool
Timer::ResetTimeout(uint64_t timeout)
{
    uint64_t old = m_timeout.load();
    if(!m_timeout.compare_exchange_strong(old, timeout)) return false;
    old = m_deadline.load();
    if(!m_deadline.compare_exchange_strong(old, utils::GetCurrentTimeMs() + timeout)) return false;
    m_success = false;
    return true;
}

bool Timer::ResetTimeout(uint64_t timeout, std::function<void(std::weak_ptr<details::TimeEvent>, Timer::ptr)> &&func)
{
    if(!ResetTimeout(timeout)) return false;
    m_callback = std::move(func);
    return true;
}

void
Timer::Execute(std::weak_ptr<details::TimeEvent> event)
{
    if (m_cancle.load())
        return;
    m_callback(event, shared_from_this());
    m_success.store(true);
}

bool
Timer::Cancle()
{
    bool old = m_cancle.load();
    if(old == true) return false;
    return m_cancle.compare_exchange_strong(old, true); 
}

// 是否已经完成
bool
Timer::IsSuccess() const
{
    return m_success.load();
}

bool
Timer::TimerCompare::operator()(const Timer::ptr &a, const Timer::ptr &b) const
{
    if (a->GetDeadline() > b->GetDeadline())
    {
        return true;
    }
    return false;
}

}


namespace galay::details
{



#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
TimeEventIDStore::TimeEventIDStore(const int capacity)
{
    m_temp = static_cast<int*>(calloc(capacity, sizeof(int)));
    for(int i = 0; i < capacity; i++){
        m_temp[i] = i;
    }
    m_capacity = capacity;
    m_eventIds.enqueue_bulk(m_temp, capacity);
    free(m_temp);
}

bool TimeEventIDStore::GetEventId(int& id)
{
    return m_eventIds.try_dequeue(id);
}

bool TimeEventIDStore::RecycleEventId(const int id)
{
    return m_eventIds.enqueue(id);
}


TimeEventIDStore TimeEvent::g_idStore(DEFAULT_TIMEEVENT_ID_CAPACITY);

bool TimeEvent::CreateHandle(GHandle &handle)
{
    return g_idStore.GetEventId(handle.fd);
}


#elif defined(__linux__)

bool TimeEvent::CreateHandle(GHandle& handle)
{
    handle.fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    return handle.fd != -1;
}

#endif


TimeEvent::TimeEvent(const GHandle handle, EventEngine* engine)
    : m_handle(handle), m_engine(engine)
{
#if defined(__linux__)
    m_engine.load()->AddEvent(this, nullptr);
#endif
}

void TimeEvent::HandleEvent(EventEngine *engine)
{
#if defined(__linux__)
    std::vector<galay::Timer::ptr> timers;
    std::unique_lock lock(this->m_mutex);
    while (! m_timers.empty() && m_timers.top()->GetDeadline()  <= GetCurrentTimeMs() ) {
        auto timer = m_timers.top();
        m_timers.pop();
        timers.emplace_back(timer);
    }
    lock.unlock();
    for (auto timer: timers)
    {
        timer->Execute(weak_from_this());
    }
    UpdateTimers();
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    std::unique_lock lock(this->m_mutex);
    Timer::ptr timer = m_timers.top();
    m_timers.pop();
    lock.unlock();
    timer->Execute(weak_from_this());
#endif
}

bool TimeEvent::SetEventEngine(EventEngine *engine)
{
    details::EventEngine* t = m_engine.load();
    if(!m_engine.compare_exchange_strong(t, engine)) {
        return false;
    }
    return true;
}

EventEngine *TimeEvent::BelongEngine()
{
    return m_engine;
}

Timer::ptr TimeEvent::AddTimer(uint64_t timeout, std::function<void(std::weak_ptr<details::TimeEvent>, Timer::ptr)> &&func)
{
    auto timer = std::make_shared<Timer>(timeout, std::move(func));
    timer->ResetTimeout(timeout);
    timer->m_cancle.store(false);
    std::unique_lock<std::shared_mutex> lock(this->m_mutex);
    this->m_timers.push(timer);
    lock.unlock();
#if defined(__linux__)
    UpdateTimers();
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    m_engine.load()->ModEvent(this, timer.get());
#endif
    return timer;
}

void TimeEvent::AddTimer(const uint64_t timeout, const galay::Timer::ptr& timer)
{
    timer->ResetTimeout(timeout);
    timer->m_cancle.store(false);
    std::unique_lock lock(this->m_mutex);
    this->m_timers.push(timer);
    lock.unlock();
#if defined(__linux__)
    UpdateTimers();
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    m_engine.load()->ModEvent(this, timer.get());
#endif
}


TimeEvent::~TimeEvent()
{
    m_engine.load()->DelEvent(this, nullptr);
#if defined(__linux__)
    close(m_handle.fd);
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    g_idStore.RecycleEventId(m_handle.fd);
#endif
}

#if defined(__linux__)
void TimeEvent::UpdateTimers()
{
    struct timespec abstime;
    std::shared_lock lock(this->m_mutex);
    if (m_timers.empty())
    {
        abstime.tv_sec = 0;
        abstime.tv_nsec = 0;
    }
    else
    {
        auto timer = m_timers.top();
        lock.unlock();
        uint64_t time = timer->GetRemainTime();
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
    timerfd_settime(this->m_handle.fd, 0, &its, nullptr);
}
#endif

}