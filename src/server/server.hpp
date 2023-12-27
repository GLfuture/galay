#ifndef GALAY_SERVER_HPP
#define GALAY_SERVER_HPP

#include "../config/config.h"
#include "../kernel/iofunction.h"
#include "../kernel/error.h"
#include "../kernel/engine.h"
#include "../kernel/basic_concepts.h"
#include "../kernel/task.h"
#include <functional>
#include <concepts>
#include <unordered_map>
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
            m_stop.store(false, std::memory_order::relaxed);
            this->m_tasks.reserve(config->m_default_fd_num);
        }

        virtual void start(std::function<void(std::shared_ptr<Task<REQ,RESP>>)> &&func) = 0;

        virtual int get_error()
        {
            return this->m_error;
        }

        virtual void stop()
        {
            this->m_stop.store(true, std::memory_order::relaxed);
        }

        Engine::ptr get_engine()
        {
            return this->m_engine;
        }

        virtual ~Server()
        {
            this->m_engine->stop();
        }

    protected:
        int m_fd = 0;
        Config::ptr m_config;
        int m_error = error::base_error::GY_SUCCESS;
        std::atomic_bool m_stop;
        Engine::ptr m_engine;
        std::unordered_map<int, std::shared_ptr<Task<REQ,RESP>>> m_tasks;
    };

    
    // tcp server
    template <Request REQ, Response RESP>
    class Tcp_Server : public Server<REQ, RESP>
    {
    public:
        using ptr = std::shared_ptr<Tcp_Server>;
        Tcp_Server() = delete;
        Tcp_Server(Tcp_Server_Config::ptr config) : Server<REQ, RESP>(config)
        {
            switch (config->m_engine)
            {
            case IO_ENGINE::IO_POLL:
            {
                break;
            }
            case IO_ENGINE::IO_SELECT:
                break;
            case IO_ENGINE::IO_EPOLL:
            {
                this->m_engine = std::make_shared<Epoll_Engine>(config->m_event_size,config->m_event_time_out);
                break;
            }
            case IO_ENGINE::IO_URING:
                break;
            default:
                this->m_error = error::server_error::GY_ENGINE_CHOOSE_ERROR;
                break;
            }
        }

        Tcp_Server(Tcp_Server_Config &&config) : Server<REQ, RESP>(std::make_shared<Tcp_Server_Config>(config))
        {
            switch (config.m_engine)
            {
            case IO_ENGINE::IO_POLL:
            {
                break;
            }
            case IO_ENGINE::IO_SELECT:
                break;
            case IO_ENGINE::IO_EPOLL:
            {
                this->m_engine = std::make_shared<Epoll_Engine>(config.m_event_size,config.m_event_time_out);
                break;
            }
            case IO_ENGINE::IO_URING:
                break;
            default:
                this->m_error = error::server_error::GY_ENGINE_CHOOSE_ERROR;
                break;
            }
        }

        void start(std::function<void(std::shared_ptr<Task<REQ,RESP>>)> &&func) override
        {
            Tcp_Server_Config::ptr config = std::dynamic_pointer_cast<Tcp_Server_Config>(this->m_config);
            this->m_error = init(this->m_fd, config);
            if (this->m_error != error::base_error::GY_SUCCESS)
                return;
            this->m_engine->add_event(this->m_fd, EPOLLIN | EPOLLET);
            this->m_tasks.emplace(std::make_pair(this->m_fd,std::make_shared<Tcp_Accept_Task<REQ,RESP>>(this->m_fd,this->m_engine,&this->m_tasks
                ,std::forward<std::function<void(std::shared_ptr<Task<REQ,RESP>>)>>(func),config->m_recv_len)));
            while (1)
            {
                int ret = this->m_engine->event_check();
                if (this->m_stop)
                    break;
                if (ret == -1)
                {
                    // need to call engine's get_error
                    this->m_error = error::server_error::GY_ENGINE_HAS_ERROR;
                    return;
                }
                epoll_event *events = (epoll_event*)this->m_engine->result();
                int nready = this->m_engine->get_active_event_num();
                for (int i = 0; i < nready; i++)
                {
                    typename Task<REQ,RESP>::ptr task = this->m_tasks[events[i].data.fd];
                    task->exec();
                    if(task->get_state() == task_state::GY_TASK_STOP){
                        this->m_tasks.erase(events[i].data.fd);
                    }
                }
            }
        }

        ~Tcp_Server()
        {
            for(auto it = this->m_tasks.begin() ; it != this->m_tasks.end() ; it++)
            {
                this->m_engine->del_event(it->first,EPOLLIN);
                close(it->first);
                it->second.reset();
            }
            this->m_tasks.clear();
        }

    protected:
        int init(int &sockfd, Tcp_Server_Config::ptr config)
        {
            sockfd = iofunction::Tcp_Function::Sock();
            if (sockfd <= 0)
            {
                return error::server_error::GY_SOCKET_ERROR;
            }
            int ret = iofunction::Tcp_Function::Bind(sockfd, config->m_port);
            if (ret == -1)
            {
                close(sockfd);
                return error::server_error::GY_BIND_ERROR;
            }
            ret = iofunction::Tcp_Function::Listen(sockfd, config->m_backlog);
            if (ret == -1)
            {
                close(sockfd);
                return error::server_error::GY_LISTEN_ERROR;
            }
            iofunction::Tcp_Function::IO_Set_No_Block(sockfd);
            return error::base_error::GY_SUCCESS;
        }

    protected:
        SSL_CTX *m_ctx;
    };

    // to do
    template <Request REQ, Response RESP>
    class Udp_Server : public Server<REQ, RESP>
    {
    public:
    };

}

#endif