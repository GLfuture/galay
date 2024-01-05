#ifndef GALAY_SERVER_HPP
#define GALAY_SERVER_HPP

#include "../config/config.h"
#include "../kernel/iofunction.h"
#include "../kernel/error.h"
#include "../kernel/scheduler.h"
#include <functional>
#include <atomic>
#include <type_traits>

namespace galay
{
    
    // server
    template <Request REQ, Response RESP>
    class Server
    {
    public:
        Server(Config::ptr config) : m_config(config)
        {
        }

        virtual void start(std::function<Task<>(std::shared_ptr<Task_Base<REQ,RESP>>)> &&func) = 0;

        virtual int get_error()
        {
            return this->m_error;
        }
    
        virtual void stop()
        {
            this->m_scheduler->stop();
        }

        IO_Scheduler<REQ,RESP>::ptr get_scheduler()
        {
            return this->m_scheduler;
        }

        virtual ~Server()
        {
            if(!this->m_scheduler->is_stop()){
                this->m_scheduler->stop();
            }
        }

    protected:
        int m_fd = 0;
        Config::ptr m_config;
        int m_error = error::base_error::GY_SUCCESS;
        IO_Scheduler<REQ,RESP>::ptr m_scheduler;
    };

    
    
    // to do
    template <Request REQ, Response RESP>
    class Udp_Server : public Server<REQ, RESP>
    {
    public:
    };

}

#endif