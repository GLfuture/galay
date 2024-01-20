#include "timer.h"
#include "base.h"

uint32_t galay::Timer::m_global_timerid = 0;

//timer
galay::Timer::Timer(uint64_t during_time, uint32_t exec_times, std::function<void()> &&func)
{
    m_global_timerid++;
    this->m_timerid = m_global_timerid;
    this->m_exec_times = exec_times;
    this->m_func = func;
    set_during_time(during_time);
}

uint64_t galay::Timer::get_current_time()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

uint64_t galay::Timer::get_during_time()
{
    return this->m_during_time;
}

uint64_t galay::Timer::get_expired_time()
{
    return this->m_expired_time;
}

uint64_t galay::Timer::get_remain_time()
{
    int64_t time = this->m_expired_time - Timer::get_current_time();
    return time < 0 ? 0 : time;
}

uint16_t galay::Timer::get_timerid()
{
    return this->m_timerid;
}

void galay::Timer::set_during_time(uint64_t during_time)
{
    this->m_during_time = during_time;
    this->m_expired_time = Timer::get_current_time() + during_time;
}

uint32_t &galay::Timer::get_exec_times()
{
    return this->m_exec_times;
}

void galay::Timer::exec()
{
    this->m_func();
}

//timermanager
galay::Timer_Manager::Timer_Manager(std::weak_ptr<Scheduler_Base> scheduler)
{
    this->m_timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
    this->m_scheduler = scheduler;
}

void galay::Timer_Manager::update_time()
{
    struct timespec abstime;
    if (m_timers.empty())
    {
        abstime.tv_sec = 0;
        abstime.tv_nsec = 0;
    }
    else
    {
        int64_t time = m_timers.top()->get_remain_time();
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
    timerfd_settime(this->m_timerfd, 0, &its, nullptr);
}

void galay::Timer_Manager::add_timer(Timer::ptr timer)
{
    std::unique_lock<std::shared_mutex> lock(this->m_mtx);
    this->m_timers.push(timer);
}

galay::Timer::ptr galay::Timer_Manager::get_ealist_timer()
{
    if (this->m_timers.empty())
        return nullptr;
    std::unique_lock<std::shared_mutex> lock(this->m_mtx);
    auto timer = this->m_timers.top();
    this->m_timers.pop();
    if (--timer->get_exec_times() > 0)
    {
        timer->set_during_time(timer->get_during_time());
        this->m_timers.push(timer);
    }
    return timer;
}

int galay::Timer_Manager::get_timerfd(){ 
    return this->m_timerfd;  
}

galay::Timer_Manager::~Timer_Manager()
{

}