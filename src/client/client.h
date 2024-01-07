#ifndef GALAY_CLIENT_H
#define GALAY_CLIENT_H

#include "../config/config.h"
#include "../kernel/basic_concepts.h"
#include "../kernel/scheduler.h"

namespace galay
{
    class Client
    {
    public:
        Client(IO_Scheduler::wptr scheduler)
            :m_scheduler(scheduler)
        {
            
        }

        int get_error()
        {
            return this->m_error;
        }

        virtual ~Client(){
            
        }
    protected:

        void add_task(Task_Base::ptr task)
        {
            if(!m_scheduler.expired())
            {
                auto scheduler = m_scheduler.lock();
                auto it = scheduler->m_tasks->find(this->m_fd);
                if (it == scheduler->m_tasks->end())
                {
                    scheduler->m_tasks->emplace(std::make_pair(this->m_fd, task));
                }
                else
                {
                    it->second = task;
                }
            }
        }
    protected:
        int m_fd;
        int m_error;
        IO_Scheduler::wptr m_scheduler;
    };



}



#endif