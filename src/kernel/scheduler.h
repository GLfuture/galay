#ifndef GALAY_SCHEDULER_H
#define GALAY_SCHEDULER_H

#include <unordered_map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include "engine.h"
#include "task.h"

namespace galay{
    class Task_Base;

    class Timer_Manager;

    class Scheduler_Base
    {
    public:
        using ptr = std::shared_ptr<Scheduler_Base>;
        virtual ~Scheduler_Base(){}
    };

    class IO_Scheduler: public Scheduler_Base , public std::enable_shared_from_this<IO_Scheduler>
    {
    public:
        using ptr = std::shared_ptr<IO_Scheduler>;
        using wptr = std::weak_ptr<IO_Scheduler>;
        IO_Scheduler(IO_ENGINE engine_type,int max_event,int timeout);

        int start();

        void stop();

        bool is_stop();

        std::shared_ptr<Engine> get_engine();
        
        std::shared_ptr<Timer_Manager> get_timer_manager();

        void add_task(std::pair<int,std::shared_ptr<Task_Base>>&& pair);

        void del_task(int fd);

        virtual ~IO_Scheduler() {}
    protected: 
        std::shared_mutex m_mtx;
        Engine::ptr m_engine;
        std::shared_ptr<std::unordered_map<int, std::shared_ptr<Task_Base>>> m_tasks;
        bool m_stop = false;
        std::shared_ptr<Timer_Manager> m_timer_manager = nullptr;
    };

}




#endif