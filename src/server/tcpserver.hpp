#ifndef GALAY_TCP_SERVER_HPP
#define GALAY_TCP_SERVER_HPP

#include "server.hpp"
#include "../kernel/tcptask.h"

namespace galay
{
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
                auto engine = std::make_shared<Epoll_Engine>(config->m_event_size, config->m_event_time_out);
                this->m_scheduler = std::make_shared<IO_Scheduler<REQ,RESP>>(engine);
                break;
            }
            case IO_ENGINE::IO_URING:
                break;
            default:
                this->m_error = error::server_error::GY_ENGINE_CHOOSE_ERROR;
                break;
            }
        }

        // 移除右值构造，通过工厂模式统一创建config
        // Tcp_Server(Tcp_Server_Config &&config) : Server<REQ, RESP>(std::make_shared<Tcp_Server_Config>(config))
        // {
        //     switch (config.m_engine)
        //     {
        //     case IO_ENGINE::IO_POLL:
        //     {
        //         break;
        //     }
        //     case IO_ENGINE::IO_SELECT:
        //         break;
        //     case IO_ENGINE::IO_EPOLL:
        //     {
        //         this->m_scheduler->m_engine = std::make_shared<Epoll_Engine>(config.m_event_size, config.m_event_time_out);
        //         break;
        //     }
        //     case IO_ENGINE::IO_URING:
        //         break;
        //     default:
        //         this->m_error = error::server_error::GY_ENGINE_CHOOSE_ERROR;
        //         break;
        //     }
        //     if(config.m_is_ssl){
        //         this->m_ctx = iofunction::Tcp_Function::SSL_Init(config.m_ssl_min_version,config.m_ssl_max_version);
        //     }
        // }

        void start(std::function<void(std::shared_ptr<Task<REQ, RESP>>)> &&func) override
        {
            Tcp_Server_Config::ptr config = std::dynamic_pointer_cast<Tcp_Server_Config>(this->m_config);
            this->m_error = init(config);
            if (this->m_error != error::base_error::GY_SUCCESS)
                return;
            add_accept_task(std::forward<std::function<void(std::shared_ptr<Task<REQ, RESP>>)>>(func),config->m_recv_len);
            while (1)
            {
                int ret = this->m_scheduler->m_engine->event_check();
                if (this->m_stop)
                    break;
                if (ret == -1)
                {
                    // need to call engine's get_error
                    this->m_error = error::server_error::GY_ENGINE_HAS_ERROR;
                    return;
                }
                epoll_event *events = (epoll_event *)this->m_scheduler->m_engine->result();
                int nready = this->m_scheduler->m_engine->get_active_event_num();
                for (int i = 0; i < nready; i++)
                {
                    typename Task<REQ, RESP>::ptr task = this->m_scheduler->m_tasks->at(events[i].data.fd);
                    task->exec();
                    if (task->get_state() == task_state::GY_TASK_STOP)
                    {
                        this->m_scheduler->m_tasks->erase(events[i].data.fd);
                    }
                }
            }
        }

        virtual ~Tcp_Server()
        {
            for (auto it = this->m_scheduler->m_tasks->begin(); it != this->m_scheduler->m_tasks->end(); it++)
            {
                this->m_scheduler->m_engine->del_event(it->first, EPOLLIN);
                close(it->first);
                it->second.reset();
            }
            this->m_scheduler->m_tasks->clear();
        }

    protected:
        int init(Tcp_Server_Config::ptr config)
        {
            this->m_fd = iofunction::Tcp_Function::Sock();
            if (this->m_fd <= 0)
            {
                return error::base_error::GY_SOCKET_ERROR;
            }
            int ret = iofunction::Tcp_Function::Reuse_Fd(this->m_fd);
            if(ret == -1){
                close(this->m_fd);
                return error::server_error::GY_SETSOCKOPT_ERROR;
            }
            ret = iofunction::Tcp_Function::Bind(this->m_fd, config->m_port);
            if (ret == -1)
            {
                close(this->m_fd);
                return error::server_error::GY_BIND_ERROR;
            }
            ret = iofunction::Tcp_Function::Listen(this->m_fd, config->m_backlog);
            if (ret == -1)
            {
                close(this->m_fd);
                return error::server_error::GY_LISTEN_ERROR;
            }
            iofunction::Tcp_Function::IO_Set_No_Block(this->m_fd);
            return error::base_error::GY_SUCCESS;
        }

        virtual void add_accept_task(std::function<void(std::shared_ptr<Task<REQ, RESP>>)> &&func , uint32_t recv_len)
        {
            this->m_scheduler->m_engine->add_event(this->m_fd, EPOLLIN | EPOLLET);
            this->m_scheduler->m_tasks->emplace(std::make_pair(this->m_fd, std::make_shared<Tcp_Accept_Task<REQ, RESP>>(this->m_fd, this->m_scheduler
                , std::forward<std::function<void(std::shared_ptr<Task<REQ, RESP>>)>>(func), recv_len)));
        }
    };

    template <Request REQ, Response RESP>
    class Tcp_SSL_Server: public Tcp_Server<REQ,RESP>
    {
    public:
        Tcp_SSL_Server(Tcp_SSL_Server_Config::ptr config):
            Tcp_Server<REQ,RESP>(config)
        {
            m_ctx = iofunction::Tcp_Function::SSL_Init_Server(config->m_ssl_min_version,config->m_ssl_max_version);
            if(m_ctx == nullptr){
                this->m_error = error::server_error::GY_SSL_CTX_INIT_ERROR;
                ERR_print_errors_fp(stderr);
                exit(-1);
            }
            if(iofunction::Tcp_Function::SSL_Config_Cert_And_Key(m_ctx,config->m_cert_filepath.c_str(),config->m_key_filepath.c_str()) == -1)
            {
                this->m_error = error::server_error::GY_SSL_CRT_OR_KEY_FILE_ERROR;
                ERR_print_errors_fp(stderr);
                exit(-1);
            }
        }

        ~Tcp_SSL_Server() override
        {
            if(this->m_ctx){
                iofunction::Tcp_Function::SSL_Destory({},m_ctx);
                this->m_ctx = nullptr;
            }
        }

    protected:
        void add_accept_task(std::function<void(std::shared_ptr<Task<REQ, RESP>>)> &&func , uint32_t recv_len) override
        {
            this->m_scheduler->m_engine->add_event(this->m_fd, EPOLLIN | EPOLLET);
            Tcp_SSL_Server_Config::ptr config = std::dynamic_pointer_cast<Tcp_SSL_Server_Config>(this->m_config);
            this->m_scheduler->m_tasks->emplace(std::make_pair(this->m_fd, std::make_shared<Tcp_SSL_Accept_Task<REQ, RESP>>(this->m_fd, this->m_scheduler
                , std::forward<std::function<void(std::shared_ptr<Task<REQ, RESP>>)>>(func), recv_len,config->m_ssl_accept_retry,this->m_ctx)));
        }
    protected:
        SSL_CTX *m_ctx = nullptr;
    };



}

#endif