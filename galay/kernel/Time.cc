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

bool Timer::ResetTimeout(int64_t timeout, timer_callback &&func)
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


TimeEvent::TimeEvent(const GHandle handle, EventEngine* engine, TimerManagerType type)
    : m_handle(handle), m_engine(engine)
{
    switch (type)
    {
    case TimerManagerType::kTimerManagerTypePriorityQueue :
        m_timer_manager = std::make_shared<PriorityQueueTimerManager>();
        break;
    case TimerManagerType::kTimerManagerTypeRbTree :
        /*To Do*/
        break;
    case TimerManagerType::kTimerManagerTypeTimeWheel :
        /*To Do*/
        break;
    default:
        break;
    }
#if defined(__linux__)
    engine->AddEvent(this, nullptr);
#endif
}

int64_t TimeEvent::OnceLoopTimeout()
{
    return m_timer_manager->OnceLoopTimeout();
}

void TimeEvent::HandleEvent(EventEngine *engine)
{
    std::list<Timer::ptr> timers = m_timer_manager->GetArriveredTimers();
    for (auto timer: timers)
    {
        timer->Execute(weak_from_this());
    }
    ActiveTimerManager();
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

void TimeEvent::AddTimer(const Timer::ptr &timer, const int64_t timeout)
{
    timer->ResetTimeout(timeout);
    timer->Start();
    this->m_timer_manager->Push(timer);
    ActiveTimerManager();
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

void TimeEvent::ActiveTimerManager()
{
    switch (m_timer_manager->Type())
    {
    case TimerManagerType::kTimerManagerTypePriorityQueue :
        this->m_timer_manager->UpdateTimers(this);
        break;
    case TimerManagerType::kTimerManagerTypeRbTree :
        this->m_timer_manager->UpdateTimers(this);
        break;
    case TimerManagerType::kTimerManagerTypeTimeWheel :
        /*To Do*/
        break;
    default:
        break;
    }
}

std::list<Timer::ptr> PriorityQueueTimerManager::GetArriveredTimers()
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

TimerManagerType PriorityQueueTimerManager::Type()
{
    return TimerManagerType::kTimerManagerTypePriorityQueue;
}


Timer::ptr PriorityQueueTimerManager::Top()
{
    std::shared_lock lock(this->m_mutex);
    if (m_timers.empty())
    {
        return nullptr;
    }
    return m_timers.top();
}


bool PriorityQueueTimerManager::IsEmpty()
{
    std::shared_lock lock(m_mutex);
    return m_timers.empty();
}


void PriorityQueueTimerManager::Push(Timer::ptr timer)
{
    std::unique_lock lock(this->m_mutex);
    m_timers.push(timer);
}

void PriorityQueueTimerManager::UpdateTimers(void* ctx)
{
#ifdef __linux__
    TimeEvent* event = static_cast<TimeEvent*>(ctx);
    struct timespec abstime;
    if (IsEmpty())
    {
        abstime.tv_sec = 0;
        abstime.tv_nsec = 0;
    }
    else
    {
        auto timer = Top();
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
    TimeEvent* event = static_cast<TimeEvent*>(ctx);
    if(IsEmpty()) {
        return;
    } else {
        auto timer = Top();
        event->m_engine.load()->ModEvent(event, timer.get());
    }
#endif
}

int64_t PriorityQueueTimerManager::OnceLoopTimeout()
{
    return -1;
}
}