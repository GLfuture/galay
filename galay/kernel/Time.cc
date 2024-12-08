#include "Time.h"
#include "WaitAction.h"
#include "Event.h"
#include <chrono>
namespace galay
{

int64_t GetCurrentTimeMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

std::string GetCurrentGMTTimeString()
{
    std::time_t now = std::time(nullptr);
    std::tm *gmt_time = std::gmtime(&now);
    if (gmt_time == nullptr)
    {
        return "";
    }
    char buffer[80];
    std::strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", gmt_time);
    return buffer;
}

Timer::Timer()
    : m_timeout(0)
{
}

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
    const int64_t time = m_deadline - GetCurrentTimeMs();
    return time < 0 ? 0 : time;
}

bool
Timer::ResetTimeout(uint64_t timeout)
{
    uint64_t old = m_timeout.load();
    if(!m_timeout.compare_exchange_strong(old, timeout)) return false;
    old = m_deadline.load();
    if(!m_deadline.compare_exchange_strong(old, GetCurrentTimeMs() + timeout)) return false;
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
