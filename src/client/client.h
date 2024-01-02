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
        int m_fd;
        int m_error;
        IO_Scheduler<REQ,RESP>::ptr m_scheduler;
    };



}



#endif