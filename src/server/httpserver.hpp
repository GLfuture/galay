#ifndef GALAY_HTTP_SERVER_HPP
#define GALAY_HTTP_SERVER_HPP

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

        // Http_Server(Http_Server_Config && config):Tcp_Server<REQ, RESP>(std::make_shared<Http_Server_Config>(config))
        // {
        
        // }
    protected:
        void add_accept_task(std::function<void(std::shared_ptr<Task<REQ, RESP>>)> &&func , uint32_t recv_len) override
        {
            this->m_engine->add_event(this->m_fd, EPOLLIN | EPOLLET);
            this->m_tasks.emplace(std::make_pair(this->m_fd, std::make_shared<Http_Accept_Task<REQ, RESP>>(this->m_fd, this->m_engine, 
                &this->m_tasks, std::forward<std::function<void(std::shared_ptr<Task<REQ, RESP>>)>>(func), recv_len)));
        }
    };
}
#endif