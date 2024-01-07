#ifndef GALAY_HTTP_SERVER_HPP
#define GALAY_HTTP_SERVER_HPP

#include "../kernel/task_http.h"
#include "tcpserver.h"

namespace galay
{
    class Http_Server : public Tcp_Server
    {
    public:
        using ptr = std::shared_ptr<Http_Server>;
        Http_Server(Http_Server_Config::ptr config, IO_Scheduler::ptr scheduler):
            Tcp_Server(config,scheduler)
        {
        
        }

    protected:
        void add_accept_task(std::function<Task<>(Task_Base::wptr)> &&func , uint32_t max_recv_len) override
        {
            this->m_scheduler->m_engine->add_event(this->m_fd, EPOLLIN | EPOLLET);
            this->m_scheduler->m_tasks->emplace(std::make_pair(this->m_fd, std::make_shared<Http_Accept_Task>(this->m_fd, this->m_scheduler
                , std::forward<std::function<Task<>(Task_Base::wptr)>>(func), max_recv_len)));
        }
    };


    class Https_Server: public Tcp_SSL_Server
    {
    public:
        using ptr = std::shared_ptr<Https_Server>;
        Https_Server(Https_Server_Config::ptr config , IO_Scheduler::ptr scheduler)
            :Tcp_SSL_Server(config,scheduler)
        {

        }
    protected:
        //添加accept task
        void add_accept_task(std::function<Task<>(Task_Base::wptr)> &&func , uint32_t max_recv_len) override
        {
            this->m_scheduler->m_engine->add_event(this->m_fd, EPOLLIN | EPOLLET);
            Https_Server_Config::ptr config = std::dynamic_pointer_cast<Https_Server_Config>(this->m_config);
            this->m_scheduler->m_tasks->emplace(std::make_pair(this->m_fd, std::make_shared<Https_Accept_Task>(this->m_fd, this->m_scheduler
            , std::forward<std::function<Task<>(Task_Base::wptr)>>(func), max_recv_len,config->m_ssl_accept_retry,this->m_ctx)));
        }
    };

}



#endif