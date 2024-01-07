#ifndef GALAY_SCHEDULER_H
#define GALAY_SCHEDULER_H

#include <unordered_map>
#include <memory>
#include "engine.h"
#include "task.h"

namespace galay{

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
        IO_Scheduler(IO_ENGINE engine_type,int max_event,int timeout)
        {
            switch (engine_type)
            {
            case IO_ENGINE::IO_POLL:
            {
                break;
            }
            case IO_ENGINE::IO_SELECT:
                break;
            case IO_ENGINE::IO_EPOLL:
            {
                this->m_engine = std::make_shared<Epoll_Engine>(max_event,timeout);
                break;
            }
            case IO_ENGINE::IO_URING:
                break;
            }
            this->m_tasks = std::make_shared<std::unordered_map<int, std::shared_ptr<Task_Base>>>();
        }

        int start()
        {
            while (1)
            {
                int ret = this->m_engine->event_check();
                if (this->m_stop)
                    break;
                if (ret == -1)
                {
                    return error::scheduler_error::GY_SCHEDULER_ENGINE_CHECK_ERROR;
                }
                epoll_event *events = (epoll_event *)this->m_engine->result();
                int nready = this->m_engine->get_active_event_num();
                for (int i = 0; i < nready; i++)
                {
                    Task_Base::ptr task = this->m_tasks->at(events[i].data.fd);
                    task->exec();
                }
            }
            return error::base_error::GY_SUCCESS;
        }

        void stop()
        {
            if(!this->m_stop)
            {
                this->m_stop = true;
                for (auto it = this->m_tasks->begin(); it != this->m_tasks->end(); it++)
                {
                    this->m_engine->del_event(it->first, EPOLLIN);
                    close(it->first);
                    it->second.reset();
                }
                this->m_tasks->clear();
                this->m_engine->stop();
            }
        }

        virtual ~IO_Scheduler()
        {
            
        }

        bool is_stop(){
            return this->m_stop;
        }

        bool m_stop = false;
        Engine::ptr m_engine;
        std::shared_ptr<std::unordered_map<int, std::shared_ptr<Task_Base>>> m_tasks;
    };

}




#endif