#ifndef GALAY_CLIENT_H
#define GALAY_CLIENT_H

#include "../config/config.h"
#include "../kernel/basic_concepts.h"
#include "../kernel/scheduler.h"

namespace galay
{
    template<Request REQ,Response RESP>
    class Client
    {
    public:
        Client(IO_Scheduler<REQ,RESP>::ptr scheduler)
            :m_scheduler(scheduler)
        {
            
        }

        int get_error()
        {
            return this->m_error;
        }

        virtual ~Client(){}
    protected:

        void add_task(Task_Base<REQ,RESP>::ptr task)
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
        IO_Scheduler<REQ,RESP>::ptr m_scheduler;
    };



}



#endif