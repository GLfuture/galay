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
        Client()
        {
            
        }
    protected:
        int m_fd;
        Engine::ptr m_engine;

    };



}



#endif