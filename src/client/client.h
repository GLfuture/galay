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
        Client(IO_Scheduler::ptr scheduler)
            :m_scheduler(scheduler)
        {
            
        }

        int get_error()
        {
            return this->m_error;
        }

        virtual ~Client(){
            if(this->m_scheduler && !this->m_scheduler->is_stop()){
                this->m_scheduler->stop();
                this->m_scheduler.reset();
            }
        }
    protected:

        void add_task(Task_Base::ptr task)
        {
            auto it = this->m_scheduler->m_tasks->find(this->m_fd);
            if(it == this->m_scheduler->m_tasks->end())
            {
                this->m_scheduler->m_tasks->emplace(std::make_pair(this->m_fd,task));
            }else{
                it->second = task;
            }
        }
    protected:
        int m_fd;
        int m_error;
        IO_Scheduler::ptr m_scheduler;
    };



}



#endif