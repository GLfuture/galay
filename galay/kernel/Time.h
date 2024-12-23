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

#endif