#ifndef GALAY_SCHEDULER_H
#define GALAY_SCHEDULER_H

#include <unordered_map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#ifdef __linux__
#include <sys/epoll.h>
#endif

#include "task.h"

namespace galay{
    class Task_Base;

    class Timer_Manager;

    class Scheduler_Base
    {
    public:
        using ptr = std::shared_ptr<Scheduler_Base>;
        using wptr = std::weak_ptr<Scheduler_Base>;

        virtual void add_task(std::pair<int,std::shared_ptr<Task_Base>>&& pair) = 0;

        virtual void del_task(int fd) = 0;

        virtual int add_event(int fd ,uint32_t event_type) = 0;
        
        virtual int del_event(int fd, uint32_t event_type) = 0;

        virtual int mod_event(int fd, uint32_t event_type) = 0;

        virtual ~Scheduler_Base(){}
    };

    class Epoll_Scheduler: public Scheduler_Base , public std::enable_shared_from_this<Epoll_Scheduler>
    {
    public:
        using ptr = std::shared_ptr<Epoll_Scheduler>;
        using wptr = std::weak_ptr<Epoll_Scheduler>;
        Epoll_Scheduler(int max_event,int timeout);

        int start();

        void stop();

        bool is_stop();
        
        std::shared_ptr<Timer_Manager> get_timer_manager();

        void add_task(std::pair<int,std::shared_ptr<Task_Base>>&& pair) override;

        void del_task(int fd) override;

        int add_event(int fd ,uint32_t event_type) override; 
        
        int del_event(int fd, uint32_t event_type) override;

        int mod_event(int fd, uint32_t event_type) override;

        // return epollfd
        int get_epoll_fd() const;

        // return events' size
        int get_event_size() const;

        virtual ~Epoll_Scheduler() {}
    protected: 
        std::shared_mutex m_mtx;
        std::shared_ptr<std::unordered_map<int, std::shared_ptr<Task_Base>>> m_tasks;
        bool m_stop = false;
        std::shared_ptr<Timer_Manager> m_timer_manager = nullptr;

        //epoll
        int m_epfd = 0;
        epoll_event *m_events = nullptr;
        int m_events_size;
        int m_time_out;
    };

}




#endif