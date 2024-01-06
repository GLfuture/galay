#ifndef GALAY_TASK_HTTP_H
#define GALAY_TASK_HTTP_H

#include "task_tcp.h"
namespace galay
{   
    class Http_RW_Task : public Tcp_RW_Task
    {
    public:
        using ptr = std::shared_ptr<Http_RW_Task>;
        Http_RW_Task(int fd, Engine::ptr engine, uint32_t read_len) : Tcp_RW_Task(fd, engine, read_len)
        {
        }
        

        int exec() override
        {
            switch (this->m_status)
            {
            case Task_Status::GY_TASK_READ:
            {
                if (Tcp_RW_Task::read_package() == -1)
                    return -1;
                if (Tcp_RW_Task::decode() == error::protocol_error::GY_PROTOCOL_INCOMPLETE)
                    return -1;
                Tcp_RW_Task::control_task_behavior(Task_Status::GY_TASK_WRITE);
                this->m_func(this->shared_from_this());
                Tcp_RW_Task::encode();
                break;
            }
            case Task_Status::GY_TASK_WRITE:
            {
                if (Tcp_RW_Task::send_package() == -1)
                    return -1;
                if(this->m_wbuffer.empty()) Tcp_RW_Task::control_task_behavior(Task_Status::GY_TASK_DISCONNECT);
                break;
            }
            default:
                break;
            }
            return 0;
        }
    private:
        void init_protocol_obj()
        {
            this->m_req = std::make_shared<Http_Request>();
            this->m_resp = std::make_shared<Http_Response>();
        }
    private:
        bool m_is_chunked = false;
    };

    class Http_Accept_Task : public Tcp_Accept_Task
    {
    public:
        using ptr = std::shared_ptr<Http_Accept_Task>;
        Http_Accept_Task(int fd, IO_Scheduler::ptr scheduler, std::function<Task<>(std::shared_ptr<Task_Base>)> &&func, uint32_t read_len) 
                        : Tcp_Accept_Task(fd, scheduler, std::forward<std::function<Task<>(std::shared_ptr<Task_Base>)>>(func), read_len)
        {
        }

        Task_Base::ptr create_rw_task(int connfd) override
        {

            auto task = std::make_shared<Http_RW_Task>(connfd, this->m_scheduler->m_engine, this->m_read_len);
            task->set_callback(std::forward<std::function<Task<>(std::shared_ptr<Task_Base>)>>(this->m_func));
            return task;
        }
    };

    class Https_RW_Task : public Tcp_SSL_RW_Task
    {
    public:
        using ptr = std::shared_ptr<Https_RW_Task>;
        Https_RW_Task(int fd, Engine::ptr engine, uint32_t read_len, SSL *ssl)
            : Tcp_SSL_RW_Task(fd, engine, read_len, ssl)
        {
        }


        int exec() override
        {
            switch (this->m_status)
            {
            case Task_Status::GY_TASK_READ:
            {
                if (Tcp_SSL_RW_Task::read_package() == -1)
                    return -1;
                if (Tcp_SSL_RW_Task::decode() == error::protocol_error::GY_PROTOCOL_INCOMPLETE)
                    return -1;
                Tcp_SSL_RW_Task::control_task_behavior(Task_Status::GY_TASK_WRITE);
                this->m_func(this->shared_from_this());
                Tcp_SSL_RW_Task::encode();
                break;
            }
            case Task_Status::GY_TASK_WRITE:
            {
                if (Tcp_SSL_RW_Task::send_package() == -1)
                    return -1;
                Tcp_SSL_RW_Task::control_task_behavior(Task_Status::GY_TASK_DISCONNECT);
                break;
            }
            default:
                break;
            }
            return 0;
        }
    private:
        void init_protocol_obj() override
        {
            this->m_req = std::make_shared<Http_Request>();
            this->m_resp = std::make_shared<Http_Response>();
        }
    private:
        bool m_is_chunked = false;
    };


    // https task
    class Https_Accept_Task : public Tcp_SSL_Accept_Task
    {
    public:
        using ptr = std::shared_ptr<Https_Accept_Task>;
        Https_Accept_Task(int fd, IO_Scheduler::ptr scheduler, std::function<Task<>(std::shared_ptr<Task_Base>)> &&func
                    , uint32_t read_len, uint32_t ssl_accept_max_retry, SSL_CTX *ctx) : Tcp_SSL_Accept_Task(fd,scheduler, std::forward<std::function<Task<>(std::shared_ptr<Task_Base>)>>(func), read_len, ssl_accept_max_retry, ctx) {}

    private:
        Task_Base::ptr create_rw_task(int connfd, SSL *ssl) override
        {
            auto task = std::make_shared<Https_RW_Task>(connfd, this->m_scheduler->m_engine, this->m_read_len, ssl);
            task->set_callback(std::forward<std::function<Task<>(std::shared_ptr<Task_Base>)>>(this->m_func));
            return task;
        }
    };

    
}

#endif