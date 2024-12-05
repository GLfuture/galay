#include "Time.h"
#include "WaitAction.h"
#include <chrono>
namespace galay
{

int64_t GetCurrentTime()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

std::string GetCurrentGMTTimeString()
{
    std::time_t now = std::time(nullptr);
    std::tm* gmt_time = std::gmtime(&now);
    if (gmt_time == nullptr) {
        return "";
    }
    char buffer[80];
    std::strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", gmt_time);
    return buffer;
}



Timer::Timer(const int64_t during_time, std::function<void(Timer::ptr)> &&func)
{
    this->m_rightHandle = std::forward<std::function<void(Timer::ptr)>>(func);
    SetDuringTime(during_time);
}

int64_t 
Timer::GetDuringTime() const
{
    return this->m_duringTime;
}

int64_t 
Timer::GetExpiredTime() const
{
    return this->m_expiredTime;
}

int64_t 
Timer::GetRemainTime() const
{
    const int64_t time = this->m_expiredTime - GetCurrentTime();
    return time < 0 ? 0 : time;
}

void 
Timer::SetDuringTime(int64_t duringTime)
{
    this->m_duringTime = duringTime;
    this->m_expiredTime = GetCurrentTime() + duringTime;
    this->m_success = false;
}

void 
Timer::Execute()
{
    if ( m_cancle.load() ) return;
    this->m_rightHandle(shared_from_this());
    this->m_success = true;
}

void 
Timer::Cancle()
{
    this->m_cancle = true;
}

// 是否已经完成
bool 
Timer::Success() const
{
    return this->m_success.load();
}

bool 
Timer::TimerCompare::operator()(const Timer::ptr &a, const Timer::ptr &b) const
{
    if (a->GetExpiredTime() > b->GetExpiredTime())
    {
        return true;
    }
    return false;
}


Deadline::Deadline()
    :m_last_active_time(GetCurrentTime()), m_timer(nullptr)
{
}

coroutine::Awaiter_bool Deadline::TimeOut(uint64_t timeout_ms)
{
    auto action = new action::CoroutineHandleAction([](coroutine::Coroutine* co, void* ctx){
        return false;
    });
    return {action, nullptr};
}
}
