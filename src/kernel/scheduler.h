#ifndef GALAY_SCHEDULER_H
#define GALAY_SCHEDULER_H

#include <unordered_map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#ifdef __linux__
#include <sys/epoll.h>
#endif
#include "../protocol/tcp.h"
#include "../protocol/http.h"
#include "timer.h"

namespace galay{

    enum Task_Status
    {
        GY_TASK_CONNECT,
        GY_TASK_SSL_CONNECT,
        GY_TASK_READ,
        GY_TASK_WRITE,
    };

    class Scheduler_Base;

    class Epoll_Scheduler;

    class Task_Base
    {
    public:
        using wptr = std::weak_ptr<Task_Base>;
        using ptr = std::shared_ptr<Task_Base>;

        // return -1 error 0 success
        virtual int exec() = 0;

        virtual std::shared_ptr<Epoll_Scheduler> get_scheduler()
        {
            return nullptr;
        }

        virtual Request_Base::ptr get_req()
        {
            return nullptr;
        }
        virtual Response_Base::ptr get_resp()
        {
            return nullptr;
        }

        virtual void control_task_behavior(Task_Status status) {}

        virtual int get_state() { return this->m_status; }

        virtual void finish() { this->m_is_finish = true; }

        virtual void set_ctx(void *ctx)
        {
            if (!this->m_ctx)
                this->m_ctx = ctx;
        }

        virtual void *get_ctx() { return this->m_ctx; }

        virtual void destory() { this->m_destroy = true; }

        virtual bool is_destroy() { return this->m_destroy; }

        virtual ~Task_Base() {}

    protected:
        int m_status;
        bool m_is_finish = false;
        bool m_destroy = false;
        void *m_ctx = nullptr;
    };


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

        virtual ~Epoll_Scheduler() ;
    protected: 
        std::shared_mutex m_mtx;
        std::unordered_map<int, std::shared_ptr<Task_Base>> m_tasks;
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