#ifndef GALAY_TASK_HTTP_H
#define GALAY_TASK_HTTP_H

#include "task_tcp.h"
namespace galay
{   
    class Http_RW_Task : public Tcp_RW_Task
    {
    public:
        using ptr = std::shared_ptr<Http_RW_Task>;
        Http_RW_Task(int fd, IO_Scheduler::wptr scheduler, uint32_t read_len) 
            : Tcp_RW_Task(fd, scheduler, read_len)
        {
            this->m_req = std::make_shared<Http_Request>();
            this->m_resp = std::make_shared<Http_Response>();
        }
        
        bool is_need_to_destroy() override
        {
            return Tcp_RW_Task::is_need_to_destroy();
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
                this->m_co_task = this->m_func(this->shared_from_this());
                break;
            }
            case Task_Status::GY_TASK_WRITE:
            {
                if(this->m_is_finish)
                {
                    Tcp_RW_Task::encode();
                    if (Tcp_RW_Task::send_package() == -1)
                        return -1;
                    control_task_behavior(GY_TASK_DISCONNECT);
                }
                break;
            }
            default:
                break;
            }
            return 0;
        }
        
        ~Http_RW_Task()
        {
        }
    private:
        bool m_is_chunked = false;
    };

    class Http_Accept_Task : public Tcp_Accept_Task
    {
    public:
        using ptr = std::shared_ptr<Http_Accept_Task>;
        Http_Accept_Task(int fd, IO_Scheduler::wptr scheduler, std::function<Task<>(Task_Base::wptr)> &&func, uint32_t read_len) 
                        : Tcp_Accept_Task(fd, scheduler, std::forward<std::function<Task<>(Task_Base::wptr)>>(func), read_len)
        {
        }

        Task_Base::ptr create_rw_task(int connfd) override
        {
            if(!this->m_scheduler.expired())
            {
                auto task = std::make_shared<Http_RW_Task>(connfd, this->m_scheduler, this->m_read_len);
                task->set_callback(std::forward<std::function<Task<>(Task_Base::wptr)>>(this->m_func));
                return task;
            }
            return nullptr;
        }
    };

    class Https_RW_Task : public Tcp_SSL_RW_Task
    {
    public:
        using ptr = std::shared_ptr<Https_RW_Task>;
        Https_RW_Task(int fd, IO_Scheduler::wptr scheduler, uint32_t read_len, SSL *ssl)
            : Tcp_SSL_RW_Task(fd, scheduler, read_len, ssl)
        {
            this->m_req = std::make_shared<Http_Request>();
            this->m_resp = std::make_shared<Http_Response>();
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
                this->m_co_task = this->m_func(this->shared_from_this());
                break;
            }
            case Task_Status::GY_TASK_WRITE:
            {
                if(this->m_is_finish)
                {
                    Tcp_SSL_RW_Task::encode();
                    if (Tcp_SSL_RW_Task::send_package() == -1)
                        return -1;
                    control_task_behavior(GY_TASK_DISCONNECT);
                }
                
                break;
            }
            default:
                break;
            }
            return 0;
        }

        virtual ~Https_RW_Task()
        {
        }
    private:

        bool m_is_chunked = false;
    };


    // https task
    class Https_Accept_Task : public Tcp_SSL_Accept_Task
    {
    public:
        using ptr = std::shared_ptr<Https_Accept_Task>;
        Https_Accept_Task(int fd, IO_Scheduler::wptr scheduler, std::function<Task<>(Task_Base::wptr)> &&func
                    , uint32_t read_len, uint32_t ssl_accept_max_retry, SSL_CTX *ctx) 
                        : Tcp_SSL_Accept_Task(fd,scheduler, std::forward<std::function<Task<>(Task_Base::wptr)>>(func), read_len, ssl_accept_max_retry, ctx) {}

    private:
        Task_Base::ptr create_rw_task(int connfd, SSL *ssl) override
        {
            auto task = std::make_shared<Https_RW_Task>(connfd, this->m_scheduler, this->m_read_len, ssl);
            task->set_callback(std::forward<std::function<Task<>(Task_Base::wptr)>>(this->m_func));
            return task;
        }
    };

    
}

#endif