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

    class Scheduler_Base
    {
    public:
        using ptr = std::shared_ptr<Scheduler_Base>;
        virtual ~Scheduler_Base(){}
    };

    class IO_Scheduler: public Scheduler_Base
    {
    public:
        using ptr = std::shared_ptr<IO_Scheduler>;
        using wptr = std::weak_ptr<IO_Scheduler>;
        IO_Scheduler(IO_ENGINE engine_type,int max_event,int timeout);

        int start();

        void stop();

        Engine::ptr get_engine();

        virtual ~IO_Scheduler() {}

        bool is_stop();
        
        void add_task(std::pair<int,std::shared_ptr<Task_Base>>&& pair);

        void del_task(int fd);
    protected: 
        std::shared_mutex m_mtx;
        Engine::ptr m_engine;
        std::shared_ptr<std::unordered_map<int, std::shared_ptr<Task_Base>>> m_tasks;
        bool m_stop = false;
    };

}




#endif