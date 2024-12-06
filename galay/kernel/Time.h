#ifndef GALAY_TIME_H
#define GALAY_TIME_H

#include <any>
#include <memory>
#include <atomic>
#include <string>
#include <functional>

namespace galay::details {
    class TimeEvent;
}

namespace galay::coroutine {
    class Awaiter_bool;
}

namespace galay {

extern int64_t GetCurrentTime(); 
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
        bool operator()(const Timer::ptr &a, const Timer::ptr &b) const;
    };
    Timer(int64_t during_time, std::function<void(Timer::ptr)> &&func);
    int64_t GetDuringTime() const;
    int64_t GetExpiredTime() const;
    int64_t GetRemainTime() const;
    std::any& GetContext() { return m_context; };
    void SetDuringTime(int64_t during_time);
    void Execute();
    void Cancle();
    bool Success() const;
private:
    std::any m_context;
    int64_t m_expiredTime{};
    int64_t m_duringTime{};
    std::atomic_bool m_cancle;
    std::atomic_bool m_success;
    std::function<void(Timer::ptr)> m_rightHandle;
};

class Deadline
{
public:
    Deadline();
    coroutine::Awaiter_bool TimeOut(uint64_t timeout_ms);
private:
    Timer::ptr m_timer;
    uint64_t m_last_active_time;
};

}

#endif