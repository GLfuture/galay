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


namespace galay{
    class Scheduler_Base;

    class Timer
    {
    public:
        using ptr = std::shared_ptr<Timer>;
        Timer(uint64_t during_time , uint32_t exec_times , std::function<void()> &&func);

        static uint64_t get_current_time();

        uint64_t get_during_time();

        uint64_t get_expired_time();

        uint64_t get_remain_time();

        uint16_t get_timerid();

        void set_during_time(uint64_t during_time);

        //get remained Exec times
        uint32_t &get_exec_times();

        void Exec();

        //取消任务
        void cancle();

        bool is_cancled();
        
        //是否已经完成
        bool is_finish();

    protected:
        static uint32_t m_global_timerid;
        // id
        uint32_t m_timerid; 
        //the times to Exec timer
        uint32_t m_exec_times;  
        uint64_t m_expired_time;
        uint64_t m_during_time;
        bool m_cancle = false;
        bool m_is_finish = true;
        std::function<void()> m_func;
    };
    
    class Timer_Manager
    {
    public:
        using ptr = std::shared_ptr<Timer_Manager>;
        using wptr = std::weak_ptr<Timer_Manager>;
        Timer_Manager(std::weak_ptr<Scheduler_Base> scheduler);

        void update_time();

        Timer::ptr get_ealist_timer();
        
        // during (ms) exec_times (Exec times)
        template<typename Func,typename ...Args,typename = std::enable_if_t<std::is_void_v<std::invoke_result_t<Func, Args...>>>>
        Timer::ptr add_timer(uint64_t during , uint32_t exec_times, Func&& f , Args&& ...args)
        {
            std::function<void()> func = std::bind(std::forward<Func&&>(f),std::forward<Args>(args)...);
            auto timer = std::make_shared<Timer>(during,exec_times,std::move(func));
            std::unique_lock<std::shared_mutex> lock(this->m_mtx);
            this->m_timers.push(timer);
            return timer;
        }

        //return timerfd
        int get_timerfd();

        ~Timer_Manager();

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
        std::weak_ptr<Scheduler_Base> m_scheduler;
        int m_timerfd;
        std::priority_queue<Timer::ptr, std::vector<Timer::ptr>, MyCompare> m_timers;
    };

}


#endif