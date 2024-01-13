#ifndef GALAY_TIMER_H
#define GALAY_TIMER_H

#include <queue>
#include <unistd.h>
#include <iostream>
#include <chrono>
#include <memory>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <sys/timerfd.h>
#include "scheduler.h"

namespace galay{
    class IO_Scheduler;

    class Timer
    {
    public:
        using ptr = std::shared_ptr<Timer>;
        Timer(uint64_t during_time , std::function<void()> &&func)
        {
            m_global_timerid ++;
            this->m_timerid = m_global_timerid;
            this->m_exec_times = 1;
            this->m_func = func;
            set_during_time(during_time);
        }

        static inline uint64_t get_current_time()
        {
            return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        }

        inline uint64_t get_expired_time()
        {
            return this->m_expired_time;
        }

        inline uint64_t get_during_time()
        {
            return this->m_during_time;
        }

        inline uint16_t get_timerid()
        {
            return this->m_timerid;
        }

        inline void set_during_time(uint64_t during_time)
        {
            this->m_during_time = during_time;
            this->m_expired_time = Timer::get_current_time() + during_time;
        }

        //get remained exec times
        inline uint32_t &get_exec_times()
        {
            return this->m_exec_times;
        }

        inline void exec()
        {
            this->m_func();
        }

    protected:
        static uint32_t m_global_timerid;
        // id
        uint32_t m_timerid; 
        //the times to exec timer
        uint32_t m_exec_times;  
        uint64_t m_expired_time;
        uint64_t m_during_time;
        std::function<void()> m_func;
    };
    
    class Timer_Manager
    {
    public:
        using ptr = std::shared_ptr<Timer_Manager>;
        using wptr = std::weak_ptr<Timer_Manager>;
        Timer_Manager(std::weak_ptr<IO_Scheduler> scheduler);

        void update_time()
        {
            struct timespec abstime;
            if (m_timers.empty())
            {
                abstime.tv_sec = 0;
                abstime.tv_nsec = 0;
            }
            else
            {
                abstime.tv_sec = m_timers.top()->get_expired_time() / 1000;
                abstime.tv_nsec = (m_timers.top()->get_expired_time() % 1000) * 1000000;
            }
            struct itimerspec its = {
                .it_interval = {},
                .it_value = abstime};
        }

        Timer::ptr get_ealist_timer()
        {
            std::unique_lock lock(this->m_mtx);
            auto timer = this->m_timers.top();
            this->m_timers.pop();
            if(timer->get_exec_times()-- > 0){
                timer->set_during_time(timer->get_during_time());
                this->m_timers.push(timer);
            }
            return timer;
        }

        void add_timer(Timer::ptr timer)
        {
            std::unique_lock lock(this->m_mtx);
            this->m_timers.push(timer);
        }

    protected:
        class MyCompare
        {
        public:
            bool operator()(const Timer::ptr &a, const Timer::ptr &b)
            {
                if(a->get_expired_time() > b->get_expired_time()){
                    return true;
                }else if(a->get_expired_time() < b->get_expired_time()){
                    return false;
                } 
                return a->get_timerid() > b->get_timerid();
            }
        };
    protected:
        std::shared_mutex m_mtx;
        std::weak_ptr<IO_Scheduler> m_scheduler;
        int m_timerfd;
        std::priority_queue<Timer::ptr, std::vector<Timer::ptr>, MyCompare> m_timers;
    };

}


#endif