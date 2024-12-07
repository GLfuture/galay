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
    class Awaiter_void;
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
    Timer(uint64_t during_time, std::function<void(Timer::ptr)> &&func);
    uint64_t GetDuringTime() const;
    uint64_t GetExpiredTime() const;
    uint64_t GetRemainTime() const;
    std::any& GetContext() { return m_context; };
    void SetDuringTime(uint64_t during_time);
    void Execute();
    void Cancle();
    bool Success() const;
private:
    std::any m_context;
    uint64_t m_expiredTime{};
    uint64_t m_duringTime{};
    std::atomic_bool m_cancle{false};
    std::atomic_bool m_success{false};
    std::function<void(Timer::ptr)> m_rightHandle;
};

}

#endif