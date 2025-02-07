#include "Time.hpp"
#include "EventEngine.h"
#if defined(__linux__)
#include <sys/timerfd.h>
#elif  defined(WIN32) || defined(_WIN32) || defined(_WIN32_) || defined(WIN64) || defined(_WIN64) || defined(_WIN64_)

#endif



namespace galay
{

Timer::ptr Timer::Create(timer_callback &&func)
{
    return std::make_shared<Timer>(std::move(func));
}

Timer::Timer(timer_callback &&func)
{
    m_callback = std::move(func);
}

int64_t
Timer::GetTimeout() const
{
    return m_timeout;
}

int64_t
Timer::GetDeadline() const
{
    return m_deadline;
}

int64_t
Timer::GetRemainTime() const
{
    const int64_t time = m_deadline - utils::GetCurrentTimeMs();
    return time < 0 ? 0 : time;
}

bool
Timer::ResetTimeout(int64_t timeout)
{
    int64_t old = m_timeout.load();
    if(!m_timeout.compare_exchange_strong(old, timeout)) return false;
    old = m_deadline.load();
    if(!m_deadline.compare_exchange_strong(old, utils::GetCurrentTimeMs() + timeout)) return false;
    m_success = false;
    return true;
}

bool Timer::ResetTimeout(int64_t timeout, std::function<void(std::weak_ptr<details::AbstractTimeEvent>, Timer::ptr)> &&func)
{
    if(!ResetTimeout(timeout)) return false;
    m_callback = std::move(func);
    return true;
}

void
Timer::Execute(std::weak_ptr<details::AbstractTimeEvent> event)
{
    if (m_cancle.load())
        return;
    m_callback(event, shared_from_this());
    m_success.store(true);
}

bool Timer::Start()
{
    bool old = true;
    return m_cancle.compare_exchange_strong(old, false);
}

bool
Timer::Cancle()
{
    bool old = false;
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


TimeEventIDStore AbstractTimeEvent::g_idStore(DEFAULT_TIMEEVENT_ID_CAPACITY);

bool AbstractTimeEvent::CreateHandle(GHandle &handle)
{
    return g_idStore.GetEventId(handle.fd);
}


#elif defined(__linux__)

bool AbstractTimeEvent::CreateHandle(GHandle& handle)
{
    handle.fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    return handle.fd != -1;
}

#endif


AbstractTimeEvent::AbstractTimeEvent(const GHandle handle, EventEngine* engine)
    : m_handle(handle), m_engine(engine)
{
}

bool AbstractTimeEvent::SetEventEngine(EventEngine *engine)
{
    details::EventEngine* t = m_engine.load();
    if(!m_engine.compare_exchange_strong(t, engine)) {
        return false;
    }
    return true;
}

EventEngine *AbstractTimeEvent::BelongEngine()
{
    return m_engine;
}

}