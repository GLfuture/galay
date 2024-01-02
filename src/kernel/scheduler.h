#ifndef GALAY_SCHEDULER_H
#define GALAY_SCHEDULER_H

#include <unordered_map>
#include <memory>
#include "task.h"

namespace galay{
    class Scheduler_Base
    {
    public:
        using ptr = std::shared_ptr<Scheduler_Base>;
        virtual ~Scheduler_Base(){}
    };

    template<Request REQ,Response RESP>
    class IO_Scheduler: public Scheduler_Base
    {
    public:
        using ptr = std::shared_ptr<IO_Scheduler>;
        IO_Scheduler(Engine::ptr engine)
        {
            this->m_engine = engine;
            this->m_tasks = std::make_shared<std::unordered_map<int, std::shared_ptr<Task<REQ, RESP>>>>();
        }
        Engine::ptr m_engine;
        std::shared_ptr<std::unordered_map<int, std::shared_ptr<Task<REQ, RESP>>>> m_tasks;
    };

}




#endif