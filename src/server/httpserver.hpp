#ifndef GALAY_HTTP_SERVER_HPP
#define GALAY_HTTP_SERVER_HPP

#include "../kernel/task_http.h"
#include "tcpserver.hpp"

namespace galay
{
    template <Request REQ, Response RESP>
    class Http_Server : public Tcp_Server<REQ,RESP>
    {
    public:
        using ptr = std::shared_ptr<Http_Server<REQ,RESP>>;
        Http_Server(Http_Server_Config::ptr config):
            Tcp_Server<REQ,RESP>(config)
        {
        
        }

    protected:
        void add_accept_task(std::function<void(std::shared_ptr<Task_Base<REQ, RESP>>)> &&func , uint32_t recv_len) override
        {
            this->m_scheduler->m_engine->add_event(this->m_fd, EPOLLIN | EPOLLET);
            this->m_scheduler->m_tasks->emplace(std::make_pair(this->m_fd, std::make_shared<Http_Accept_Task<REQ, RESP>>(this->m_fd, this->m_scheduler
                , std::forward<std::function<void(std::shared_ptr<Task_Base<REQ, RESP>>)>>(func), recv_len)));
        }
    };


    template <Request REQ, Response RESP>
    class Https_Server: public Tcp_SSL_Server<REQ,RESP>
    {
    public:
        using ptr = std::shared_ptr<Https_Server>;
        Https_Server(Https_Server_Config::ptr config)
            :Tcp_SSL_Server<REQ,RESP>(config)
        {

        }
    protected:
        void add_accept_task(std::function<void(std::shared_ptr<Task_Base<REQ, RESP>>)> &&func , uint32_t recv_len) override
        {
            this->m_scheduler->m_engine->add_event(this->m_fd, EPOLLIN | EPOLLET);
            Https_Server_Config::ptr config = std::dynamic_pointer_cast<Https_Server_Config>(this->m_config);
            this->m_scheduler->m_tasks->emplace(std::make_pair(this->m_fd, std::make_shared<Https_Accept_Task<REQ, RESP>>(this->m_fd, this->m_scheduler
            , std::forward<std::function<void(std::shared_ptr<Task_Base<REQ, RESP>>)>>(func), recv_len,config->m_ssl_accept_retry,this->m_ctx)));
        }
    };

}



#endif