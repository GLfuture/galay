#ifndef GALAY_TASK_TCP_H
#define GALAY_TASK_TCP_H

#include "task.h"


namespace galay
{
    // tcp
    // server read and write task
    class Tcp_RW_Task : public Task_Base, public std::enable_shared_from_this<Tcp_RW_Task>
    {
    protected:
        using Callback = std::function<Task<>(Task_Base::wptr)>;

    public:
        using ptr = std::shared_ptr<Tcp_RW_Task>;
        Tcp_RW_Task(int fd, IO_Scheduler::wptr scheduler, uint32_t read_len)
        {
            this->m_status = Task_Status::GY_TASK_READ;
            this->m_error = error::base_error::GY_SUCCESS;
            this->m_scheduler = scheduler;
            this->m_fd = fd;
            this->m_read_len = read_len;
            this->m_temp = new char[read_len];
            this->m_req = std::make_shared<Tcp_Request>();
            this->m_resp = std::make_shared<Tcp_Response>();
        }

        void control_task_behavior(Task_Status status) override
        {
            if(!this->m_scheduler.expired())
            {
                auto scheduler = this->m_scheduler.lock();
                switch (status)
                {
                case Task_Status::GY_TASK_WRITE:
                {
                    scheduler->m_engine->mod_event(this->m_fd, EPOLLOUT);
                    this->m_status = Task_Status::GY_TASK_WRITE;
                    break;
                }
                case Task_Status::GY_TASK_READ:
                {
                    scheduler->m_engine->mod_event(this->m_fd, EPOLLIN);
                    this->m_status = Task_Status::GY_TASK_READ;
                    break;
                }
                case Task_Status::GY_TASK_DISCONNECT:
                {
                    this->m_status = Task_Status::GY_TASK_DISCONNECT;
                    break;
                }
                default:
                    break;
                }
            } 
        }

        std::string &Get_Rbuffer() { return m_rbuffer; }

        std::string &Get_Wbuffer() { return m_wbuffer; }

        Request_Base::ptr get_req() override { return this->m_req; }
        Response_Base::ptr get_resp() override { return this->m_resp; }

        // return -1 to delete this task from server
        int exec() override
        {
            switch (this->m_status)
            {
            case Task_Status::GY_TASK_READ:
            {
                if (read_package() == -1)
                    return -1;
                if (decode() == error::protocol_error::GY_PROTOCOL_INCOMPLETE)
                    return -1;
                control_task_behavior(Task_Status::GY_TASK_WRITE);
                m_co_task = this->m_func(this->shared_from_this());
                break;
            }
            case Task_Status::GY_TASK_WRITE:
            {
                if(this->m_is_finish) {
                    encode();
                    if (send_package() == -1)
                        return -1;
                    control_task_behavior(Task_Status::GY_TASK_READ);
                    this->m_is_finish = false;
                }
                break;
            }
            default:
                break;
            }
            return 0;
        }

        void set_callback(Callback &&func)
        {
            this->m_func = func;
        }

        IO_Scheduler::ptr get_scheduler() override
        {
            if(!this->m_scheduler.expired()) return this->m_scheduler.lock();
            return nullptr;
        }

        void reset_buffer(int len)
        {
            if (this->m_temp)
                delete[] this->m_temp;
            this->m_temp = new char[len];
            this->m_read_len = len;
        }

        bool is_need_to_destroy() override
        {
            return this->m_is_finish && this->m_wbuffer.empty() && this->m_status == Task_Status::GY_TASK_DISCONNECT;
        }

        virtual ~Tcp_RW_Task()
        {
            if (m_temp)
            {
                delete[] m_temp;
                m_temp = nullptr;
            }
        }

    protected:
        
        int decode()
        {
            int state = error::base_error::GY_SUCCESS;
            int len = this->m_req->decode(this->m_rbuffer, state);
            if (state == error::protocol_error::GY_PROTOCOL_INCOMPLETE)
                return state;
            this->m_rbuffer.erase(this->m_rbuffer.begin(), this->m_rbuffer.begin() + len);
            return state;
        }

        void encode()
        {
            m_wbuffer.append(m_resp->encode());
        }

        virtual int read_package()
        {
            this->m_error = error::base_error::GY_SUCCESS;
            int len = iofunction::Tcp_Function::Recv(this->m_fd, this->m_temp, this->m_read_len);
            if (len == 0)
            {
                finish();
                control_task_behavior(GY_TASK_DISCONNECT);
                return -1;
            }
            else if (len == -1)
            {
                if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
                {
                    return -1;
                }
                else
                {
                    finish();
                    control_task_behavior(GY_TASK_DISCONNECT);
                    return -1;
                }
            }
            this->m_rbuffer.append(this->m_temp, len);
            memset(this->m_temp, 0, len);
            return 0;
        }

        virtual int send_package()
        {
            int len = iofunction::Tcp_Function::Send(this->m_fd, this->m_wbuffer, this->m_wbuffer.length());
            if (len == -1)
            {
                if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
                {
                    return -1;
                }
                else
                {
                    finish();
                    control_task_behavior(GY_TASK_DISCONNECT);
                    return -1;
                }
            }
            else if(len == 0 && !this->m_wbuffer.empty())
            {
                finish();
                control_task_behavior(GY_TASK_DISCONNECT);
                return -1;
            }
            this->m_wbuffer.erase(this->m_wbuffer.begin(), this->m_wbuffer.begin() + len);
            return 0;
        }

    protected:
        char *m_temp = nullptr;
        IO_Scheduler::wptr m_scheduler;
        int m_fd;
        uint32_t m_read_len;
        std::string m_rbuffer;
        std::string m_wbuffer;
        Request_Base::ptr m_req;
        Response_Base::ptr m_resp;
        Task<> m_co_task;
        Callback m_func;
    };

    class Tcp_Accept_Task : public Task_Base
    {
    public:
        using ptr = std::shared_ptr<Tcp_Accept_Task>;
        Tcp_Accept_Task(int fd, IO_Scheduler::wptr scheduler,
                        std::function<Task<>(Task_Base::wptr)> &&func, uint32_t read_len)
        {
            this->m_fd = fd;
            this->m_status = Task_Status::GY_TASK_READ;
            this->m_error = error::base_error::GY_SUCCESS;
            this->m_scheduler = scheduler;
            this->m_func = func;
            this->m_read_len = read_len;
        }

        //
        int exec() override
        {
            int connfd = iofunction::Tcp_Function::Accept(this->m_fd);
            if (connfd <= 0)
                return -1;
            if(!this->m_scheduler.expired()){
                auto task = create_rw_task(connfd);
                add_task(connfd, task);
                iofunction::Tcp_Function::IO_Set_No_Block(connfd);
                this->m_scheduler.lock()->m_engine->add_event(connfd, EPOLLIN | EPOLLET);
            }
            return 0;
        }

        bool is_need_to_destroy() override
        {
            return this->m_is_finish;
        }

        virtual ~Tcp_Accept_Task()
        {
        }

    protected:
        virtual Task_Base::ptr create_rw_task(int connfd)
        {
            auto task = std::make_shared<Tcp_RW_Task>(connfd, this->m_scheduler, this->m_read_len);
            task->set_callback(std::forward<std::function<Task<>(Task_Base::wptr)>>(this->m_func));
            return task;
        }

        virtual void add_task(int connfd, Task_Base::ptr task)
        {
            auto scheduler = this->m_scheduler.lock();

            auto it = scheduler->m_tasks->find(connfd);
            if (it == scheduler->m_tasks->end())
            {
                scheduler->m_tasks->emplace(connfd, task);
            }
            else
            {
                it->second = task;
            }
        }

    protected:
        int m_fd;
        IO_Scheduler::wptr m_scheduler;
        uint32_t m_read_len;
        std::function<Task<>(Task_Base::wptr)> m_func;
    };

    class Tcp_SSL_RW_Task : public Tcp_RW_Task
    {
    public:
        using ptr = std::shared_ptr<Tcp_SSL_RW_Task>;
        Tcp_SSL_RW_Task(int fd, IO_Scheduler::wptr scheduler, uint32_t read_len, SSL *ssl)
            : Tcp_RW_Task(fd, scheduler, read_len), m_ssl(ssl)
        {
        }

        virtual ~Tcp_SSL_RW_Task()
        {
            if (this->m_ssl)
            {
                iofunction::Tcp_Function::SSL_Destory(this->m_ssl);
                this->m_ssl = nullptr;
            }
        }

    protected:
        int read_package() override
        {
            int len = iofunction::Tcp_Function::SSL_Recv(this->m_ssl, this->m_temp, this->m_read_len);
            if (len == 0)
            {
                finish();
                control_task_behavior(GY_TASK_DISCONNECT);
                return -1;
            }
            else if (len == -1)
            {
                if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
                {
                    return -1;
                }
                else
                {
                    finish();
                    control_task_behavior(GY_TASK_DISCONNECT);
                    return -1;
                }
            }
            this->m_rbuffer.append(this->m_temp, len);
            memset(this->m_temp, 0, len);
            return 0;
        }

        int send_package() override
        {
            int len = iofunction::Tcp_Function::SSL_Send(this->m_ssl, this->m_wbuffer, this->m_wbuffer.length());
            if (len == -1)
            {
                if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
                {
                    return -1;
                }
                else
                {
                    finish();
                    control_task_behavior(GY_TASK_DISCONNECT);
                    return -1;
                }
            }
            else if(len == 0 && !this->m_wbuffer.empty())
            {
                finish();
                control_task_behavior(GY_TASK_DISCONNECT);
                return -1;
            }
            this->m_wbuffer.erase(this->m_wbuffer.begin(), this->m_wbuffer.begin() + len);
            return 0;
        }

    protected:
        SSL *m_ssl = nullptr;
    };

    // tcp's ssl task
    class Tcp_SSL_Accept_Task : public Tcp_Accept_Task
    {
    public:
        using ptr = std::shared_ptr<Tcp_SSL_Accept_Task>;
        Tcp_SSL_Accept_Task(int fd, IO_Scheduler::wptr scheduler, std::function<Task<>(Task_Base::wptr)> &&func, 
                    uint32_t read_len, uint32_t ssl_accept_max_retry, SSL_CTX *ctx) 
                        : Tcp_Accept_Task(fd,scheduler, std::forward<std::function<Task<>(Task_Base::wptr)>>(func), read_len)
        {
            this->m_ctx = ctx;
            this->m_ssl_accept_retry = ssl_accept_max_retry;
        }

        int exec() override
        {
            int connfd = iofunction::Tcp_Function::Accept(this->m_fd);
            if (connfd <= 0)
                return -1;
            SSL *ssl = iofunction::Tcp_Function::SSL_Create_Obj(this->m_ctx, connfd);
            if(ssl == nullptr){
                close(connfd);
                return -1;
            }
            iofunction::Tcp_Function::IO_Set_No_Block(connfd);
            int ret = iofunction::Tcp_Function::SSL_Accept(ssl);
            uint32_t retry = 0;
            while (ret <= 0 && retry++ <= this->m_ssl_accept_retry)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_SSL_SLEEP_MISC_PER_RETRY));
                ret = iofunction::Tcp_Function::SSL_Accept(ssl);
                if(ret <= 0){
                    int ssl_err = SSL_get_error(ssl,ret);
                    if(ssl_err == SSL_ERROR_WANT_READ || ssl_err == SSL_ERROR_WANT_WRITE || ssl_err == SSL_ERROR_WANT_ACCEPT){
                        continue;
                    }else{
                        iofunction::Tcp_Function::SSL_Destory(ssl);
                        close(connfd);
                        return -1;
                    }
                }
            }

            if(!this->m_scheduler.expired()){
                auto task = create_rw_task(connfd, ssl);
                Tcp_Accept_Task::add_task(connfd, task);
                this->m_scheduler.lock()->m_engine->add_event(connfd, EPOLLIN | EPOLLET);
            }
            return 0;
        }

        virtual ~Tcp_SSL_Accept_Task()
        {
            if (this->m_ctx)
                this->m_ctx = nullptr;
        }

    protected:
        virtual Task_Base::ptr create_rw_task(int connfd, SSL *ssl)
        {

            auto task = std::make_shared<Tcp_SSL_RW_Task>(connfd, this->m_scheduler, this->m_read_len, ssl);
            task->set_callback(std::forward<std::function<Task<>(Task_Base::wptr)>>(this->m_func));
            return task;
        }

    protected:
        SSL_CTX *m_ctx;
        uint32_t m_ssl_accept_retry;
    };

}

#endif