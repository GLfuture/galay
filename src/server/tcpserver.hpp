#ifndef GALAY_TCP_SERVER_HPP
#define GALAY_TCP_SERVER_HPP

#include "server.hpp"
#include "../kernel/task_tcp.h"

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
            this->m_scheduler = std::make_shared<IO_Scheduler<REQ,RESP>>(config->m_engine,config->m_event_size,config->m_event_time_out);
        }

        void start(std::function<Task<>(std::shared_ptr<Task_Base<REQ, RESP>>)> &&func) override
        {
            Tcp_Server_Config::ptr config = std::dynamic_pointer_cast<Tcp_Server_Config>(this->m_config);
            this->m_error = init(config);
            if (this->m_error != error::base_error::GY_SUCCESS)
                return;
            add_accept_task(std::forward<std::function<Task<>(std::shared_ptr<Task_Base<REQ, RESP>>)>>(func),config->m_recv_len);
            this->m_error = this->m_scheduler->start();
            
        }

        virtual ~Tcp_Server()
        {
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

        virtual void add_accept_task(std::function<Task<>(std::shared_ptr<Task_Base<REQ, RESP>>)> &&func , uint32_t recv_len)
        {
            this->m_scheduler->m_engine->add_event(this->m_fd, EPOLLIN | EPOLLET);
            this->m_scheduler->m_tasks->emplace(std::make_pair(this->m_fd, std::make_shared<Tcp_Accept_Task<REQ, RESP>>(this->m_fd, this->m_scheduler
                , std::forward<std::function<Task<>(std::shared_ptr<Task_Base<REQ, RESP>>)>>(func), recv_len)));
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
        void add_accept_task(std::function<Task<>(std::shared_ptr<Task_Base<REQ, RESP>>)> &&func , uint32_t recv_len) override
        {
            this->m_scheduler->m_engine->add_event(this->m_fd, EPOLLIN | EPOLLET);
            Tcp_SSL_Server_Config::ptr config = std::dynamic_pointer_cast<Tcp_SSL_Server_Config>(this->m_config);
            this->m_scheduler->m_tasks->emplace(std::make_pair(this->m_fd, std::make_shared<Tcp_SSL_Accept_Task<REQ, RESP>>(this->m_fd, this->m_scheduler
                , std::forward<std::function<Task<>(std::shared_ptr<Task_Base<REQ, RESP>>)>>(func), recv_len,config->m_ssl_accept_retry,this->m_ctx)));
        }
    protected:
        SSL_CTX *m_ctx = nullptr;
    };



}

#endif