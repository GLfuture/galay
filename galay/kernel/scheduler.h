#ifndef GALAY_SCHEDULER_H
#define GALAY_SCHEDULER_H

#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#ifdef __linux__
#include <sys/epoll.h>
#endif
#include <sys/select.h>
#include <limits>
#include "base.h"

namespace galay{
#ifdef __linux__
    class Epoll_Scheduler: public Scheduler_Base , public std::enable_shared_from_this<Epoll_Scheduler>
    {
    public:
        using ptr = std::shared_ptr<Epoll_Scheduler>;
        using wptr = std::weak_ptr<Epoll_Scheduler>;
        Epoll_Scheduler(int max_event,int timeout);

        int Start() override;
        
        std::shared_ptr<Timer_Manager> GetTimerManager() override;

        void AddTask(std::pair<int,std::shared_ptr<Task_Base>>&& pair) override;

        void DelTask(int fd) override;
        
        int AddEvent(int fd ,int event_type) override; 
        
        int DelEvent(int fd, int event_type) override;

        //from is unuse
        int ModEvent(int fd, int from , int to) override;

        void Stop() override;

        bool IsStop() override;

        // return epollfd
        int get_epoll_fd() const;

        // return events' size
        int get_event_size() const;

        virtual ~Epoll_Scheduler() ;

    private:
        void CloseConn(int fd) override;
    private: 
        std::mutex m_mtx;
        std::unordered_map<int, std::shared_ptr<Task_Base>> m_tasks;
        bool m_stop = false;
        std::shared_ptr<Timer_Manager> m_timer_manager = nullptr;

        //epoll
        int m_epfd = 0;
        epoll_event *m_events = nullptr;
        int m_event_num;
        int m_check_timeout;
    };
#endif
    class Select_Scheduler: public Scheduler_Base , public std::enable_shared_from_this<Select_Scheduler>
    {
    public:
        using ptr = std::shared_ptr<Select_Scheduler>;
        using wptr = std::weak_ptr<Select_Scheduler>;
        Select_Scheduler(int timeout);//ms

        void AddTask(std::pair<int, std::shared_ptr<Task_Base>> &&pair) override;

        void DelTask(int fd) override;

        int AddEvent(int fd, int event_type) override;


        int DelEvent(int fd, int event_type) override;


        int ModEvent(int fd, int from ,int to) override;
        //is stoped?
        bool IsStop() override;

        int Start() override;

        std::shared_ptr<Timer_Manager> GetTimerManager() override;

        void Stop() override;

        virtual ~Select_Scheduler();
    private:
        void CloseConn(int fd) override;
    private:
        int m_maxfd = INT32_MIN;
        int m_minfd = INT32_MAX;
        std::mutex m_mtx;
        fd_set m_rfds;
        fd_set m_wfds;
        fd_set m_efds;
        bool m_stop;
        int m_check_timeout;
        std::unordered_map<int,std::shared_ptr<Task_Base>> m_tasks;
        std::shared_ptr<Timer_Manager> m_timer_manager = nullptr;
    };



}




#endif