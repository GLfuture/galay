#ifndef GALAY_TASK_H
#define GALAY_TASK_H

#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <chrono>
#include <algorithm>
#include "base.h"
#include "coroutine.h"
#include "error.h"
#include "iofunction.h"
#include "scheduler.h"


namespace galay
{

    class Timer_Manager;


    template <typename RESULT = void>
    class Task : public Coroutine<RESULT>
    {
    public:
        using promise_type = Promise<RESULT>;
        Task()
        {
        }

        Task(std::coroutine_handle<promise_type> co_handle) noexcept
            : Coroutine<RESULT>(co_handle)
        {
        }

        Task(Task<RESULT> &&other) noexcept
            : Coroutine<RESULT>(other)
        {
        }

        Task<RESULT> &operator=(const Task<RESULT> &other) = delete;

        Task<RESULT> &operator=(Task<RESULT> &&other)
        {
            this->m_handle = other.m_handle;
            other.m_handle = nullptr;
            return *this;
        }
    };

    // tcp
    // server read and write task
    template <Tcp_Request REQ, Tcp_Response RESP>
    class Tcp_RW_Task : public Task_Base, public std::enable_shared_from_this<Tcp_RW_Task<REQ,RESP>>
    {
    protected:
        using Callback = std::function<Task<>(Task_Base::wptr)>;

    public:
        using ptr = std::shared_ptr<Tcp_RW_Task>;
        Tcp_RW_Task(int fd, std::weak_ptr<Scheduler_Base> scheduler, uint32_t read_len, int conn_timeout)
        {
            this->m_status = Task_Status::GY_TASK_READ;
            this->m_error = error::base_error::GY_SUCCESS;
            this->m_scheduler = scheduler;
            this->m_fd = fd;
            this->m_read_len = read_len;
            this->m_temp = new char[read_len];
            this->m_req = std::make_shared<REQ>();
            this->m_resp = std::make_shared<RESP>();
            this->m_conn_timeout = conn_timeout;
            if(conn_timeout > 0){
                add_timeout_timer();
            }
        }

        void control_task_behavior(Task_Status status) override
        {
            if (!this->m_scheduler.expired())
            {
                auto scheduler = this->m_scheduler.lock();
                switch (status)
                {
                case Task_Status::GY_TASK_WRITE:
                {
                    if(scheduler->mod_event(this->m_fd, GY_EVENT_READ , GY_EVENT_WRITE)==-1)
                    {
                        std::cout<< "mod event failed fd = " <<this->m_fd <<'\n';
                    }
                    this->m_status = Task_Status::GY_TASK_WRITE;
                    break;
                }
                case Task_Status::GY_TASK_READ:
                {
                    if(scheduler->mod_event(this->m_fd, GY_EVENT_WRITE , GY_EVENT_READ)==-1)
                    {
                        std::cout<< "mod event failed fd = " <<this->m_fd <<'\n';
                    }
                    this->m_status = Task_Status::GY_TASK_READ;
                    break;
                }
                default:
                    break;
                }
            }
        }
        Tcp_Request_Base::ptr get_req() override { return this->m_req; }
        Tcp_Response_Base::ptr get_resp() override { return this->m_resp; }

        // return -1 to delete this task from server
        int exec() override
        {
            if (this->m_conn_timeout > 0)
            {
                this->m_timer->cancle();
                add_timeout_timer();
            }
            switch (this->m_status)
            {
            case Task_Status::GY_TASK_READ:
            {
                if (read_package() == -1)
                    return -1;
                m_co_task = this->m_func(this->shared_from_this());
                break;
            }
            case Task_Status::GY_TASK_WRITE:
            {

                encode();
                if (send_package() == -1)
                    return -1;
                if (!this->m_is_finish)
                    control_task_behavior(Task_Status::GY_TASK_READ);
                else
                    Task_Base::destory();
                break;
            }
            default:
                break;
            }
            return 0;
        }

        std::shared_ptr<Scheduler_Base> get_scheduler() override
        {
            if (!this->m_scheduler.expired())
                return this->m_scheduler.lock();
            return nullptr;
        }

        std::string &Get_Rbuffer() { return m_rbuffer; }

        std::string &Get_Wbuffer() { return m_wbuffer; }

        void set_callback(Callback &&func) { this->m_func = func; }

        void reset_buffer(int len)
        {
            if (this->m_temp)
                delete[] this->m_temp;
            this->m_temp = new char[len];
            this->m_read_len = len;
        }

        virtual int get_error() { return this->m_error; }

        virtual ~Tcp_RW_Task()
        {
            if (m_temp)
            {
                delete[] m_temp;
                m_temp = nullptr;
            }
            if (m_timer)
            {
                if (!m_timer->is_cancled())
                {
                    m_timer->cancle();
                }
            }
        }

    protected:
        void add_timeout_timer()
        {
            this->m_timer = this->m_scheduler.lock()->get_timer_manager()->add_timer(this->m_conn_timeout, 1, [this]() {
                    this->m_scheduler.lock()->del_event(this->m_fd,GY_EVENT_READ|GY_EVENT_WRITE|GY_EVENT_ERROR);
                    this->m_scheduler.lock()->del_task(this->m_fd);
                    close(this->m_fd);
                });
        }

        void encode()
        {
            m_wbuffer.append(m_resp->encode());
        }

        virtual int read_package()
        {
            switch (m_req->proto_type())
            {
                //buffer read empty
            case GY_ALL_RECIEVE_PROTOCOL_TYPE:
                return read_all_buffer();
                //fixed proto package
            case GY_PACKAGE_FIXED_PROTOCOL_TYPE:
                return read_fixed_pakage();
                //fixed head and head includes body's length
            case GY_HEAD_FIXED_PROTOCOL_TYPE:
                return read_fixed_head();
            }
            return -1;
        }

        virtual int send_package()
        {
            int len;
            if(this->m_wbuffer.empty()) return 0;
            do{
                len = iofunction::Tcp_Function::Send(this->m_fd, this->m_wbuffer, this->m_wbuffer.length());
                if (len != -1 && len != 0) this->m_wbuffer.erase(this->m_wbuffer.begin(), this->m_wbuffer.begin() + len);
                if(this->m_wbuffer.empty()) break;
            }while(len != 0 && len != -1);
            if (len == -1)
            {
                if (errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN)
                {
                    Task_Base::destory();
                    return -1;
                }
            }
            else if (len == 0)
            {
                Task_Base::destory();
                return -1;
            } 
            return 0;
        }

        int read_all_buffer()
        {
            int len;
            do
            {
                memset(this->m_temp, 0, this->m_read_len);
                len = iofunction::Tcp_Function::Recv(this->m_fd, this->m_temp, this->m_read_len);
                if (len != -1 && len != 0)
                    this->m_rbuffer.append(this->m_temp, len);
            } while (len != -1 && len != 0);
            if (len == -1)
            {
                if (errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN)
                {
                    Task_Base::destory();
                    return -1;
                }
            }
            else if (len == 0)
            {
                Task_Base::destory();
                return -1;
            }
            int status;
            len = this->m_req->decode(this->m_rbuffer, status);
            if (status == error::protocol_error::GY_PROTOCOL_INCOMPLETE)
                return -1;
            this->m_rbuffer.erase(this->m_rbuffer.begin(), this->m_rbuffer.begin() + len);
            return 0;
        }

        int read_fixed_pakage()
        {
            uint32_t remain_len = m_req->proto_fixed_len() - this->m_rbuffer.length();
            int len = 1;
            do{
                memset(this->m_temp, 0, this->m_read_len);
                len = iofunction::Tcp_Function::Recv(this->m_fd, this->m_temp, std::min(remain_len,this->m_read_len));
                if (len != -1 && len != 0)
                    this->m_rbuffer.append(this->m_temp, len);
                remain_len -= len;
            }while(len != -1 && len != 0 && remain_len > 0);
            if (len == -1)
            {
                if (errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN)
                {
                    Task_Base::destory();
                    return -1;
                }
            }
            else if (len == 0)
            {
                Task_Base::destory();
                return -1;
            }
            int status;
            len = this->m_req->decode(this->m_rbuffer, status);
            if (status == error::protocol_error::GY_PROTOCOL_INCOMPLETE)
                return -1;
            this->m_rbuffer.erase(this->m_rbuffer.begin(), this->m_rbuffer.begin() + m_req->proto_fixed_len());
            return 0;
        }

        int read_fixed_head()
        {
            uint32_t remain_len = m_req->proto_fixed_len() - this->m_rbuffer.length();
            int len = 1;
            do{
                memset(this->m_temp, 0, this->m_read_len);
                len = iofunction::Tcp_Function::Recv(this->m_fd, this->m_temp, std::min(remain_len,this->m_read_len));
                if (len != -1 && len != 0)
                    this->m_rbuffer.append(this->m_temp, len);
                remain_len -= len;
            }while(len != -1 && len != 0 && remain_len > 0);
            if (len == -1)
            {
                if (errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN)
                {
                    Task_Base::destory();
                    return -1;
                }
            }
            else if (len == 0)
            {
                Task_Base::destory();
                return -1;
            }
            int status;
            len = this->m_req->decode(this->m_rbuffer, status); //解析head 里面赋值 proto_extra_len , return head len
            remain_len = m_req->proto_extra_len() + m_req->proto_fixed_len() - this->m_rbuffer.length();
            if (remain_len != 0)
            {
                do
                {
                    memset(this->m_temp, 0, this->m_read_len);
                    len = iofunction::Tcp_Function::Recv(this->m_fd, this->m_temp, std::min(remain_len, this->m_read_len));
                    if (len != -1 && len != 0)
                        this->m_rbuffer.append(this->m_temp, len);
                    remain_len -= len;
                } while (len != -1 && len != 0 && remain_len > 0);
                if (len == -1)
                {
                    if (errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN)
                    {
                        Task_Base::destory();
                        return -1;
                    }
                }
                else if (len == 0)
                {
                    Task_Base::destory();
                    return -1;
                }
            }
            if(remain_len != 0) return -1;
            this->m_req->set_extra_msg(std::move(this->m_rbuffer.substr(m_req->proto_fixed_len())));
            this->m_rbuffer.clear();
            return 0;
        }


    protected:
        char *m_temp = nullptr;
        std::weak_ptr<Scheduler_Base> m_scheduler;
        int m_fd;
        uint32_t m_read_len;
        std::string m_rbuffer;
        std::string m_wbuffer;
        std::shared_ptr<REQ> m_req;
        std::shared_ptr<RESP> m_resp;
        Task<> m_co_task;
        Callback m_func;
        int m_error;
        //timer
        int m_conn_timeout;
        Timer::ptr m_timer;
    };

    template <Tcp_Request REQ, Tcp_Response RESP>
    class Tcp_Main_Task : public Task_Base
    {
    public:
        using ptr = std::shared_ptr<Tcp_Main_Task>;
        Tcp_Main_Task(int fd, std::vector<std::weak_ptr<Scheduler_Base>> schedulers,
                        std::function<Task<>(Task_Base::wptr)> &&func, int read_len,int conn_timeout)
        {
            this->m_fd = fd;
            this->m_status = Task_Status::GY_TASK_READ;
            this->m_schedulers = schedulers;
            this->m_func = func;
            this->m_read_len = read_len;
            this->m_conn_timeout = conn_timeout;
        }

        int exec() override
        {
            int connfd = iofunction::Tcp_Function::Accept(this->m_fd);
            if (connfd <= 0)
                return -1;
            if (this->m_keepalive_conf.m_keepalive)
            {
                int ret = iofunction::Tcp_Function::Sock_Keepalive(connfd, this->m_keepalive_conf.m_idle, this->m_keepalive_conf.m_interval, this->m_keepalive_conf.m_retry);
                if (ret == -1)
                {
                    std::cout << strerror(errno) << '\n';
                }
            }
            int indx;
            if (m_schedulers.size() == 1)
            {
                indx = 0;
            }else{
                indx = connfd % (m_schedulers.size() - 1) + 1;
            }
            if (!this->m_schedulers[indx].expired())
            {
                auto task = create_rw_task(connfd, this->m_schedulers[indx]);
                this->m_schedulers[indx].lock()->add_task({connfd, task});
                iofunction::Tcp_Function::IO_Set_No_Block(connfd);
                if (this->m_schedulers[indx].lock()->add_event(connfd, GY_EVENT_READ | GY_EVENT_EPOLLET | GY_EVENT_ERROR) == -1)
                {
                    std::cout << "add event failed fd = " << connfd << '\n';
                }
            }
            return 0;
        }

        void enable_keepalive(uint16_t idle, uint16_t interval, uint16_t retry)
        {
            Tcp_Keepalive_Config keepalive{
                .m_keepalive = true,
                .m_idle = idle,
                .m_interval = interval,
                .m_retry = retry
            };
            this->m_keepalive_conf = keepalive;
        }

        virtual ~Tcp_Main_Task()
        {

        }

    protected:
        virtual Task_Base::ptr create_rw_task(int connfd,Scheduler_Base::wptr scheduler)
        {
            auto task = std::make_shared<Tcp_RW_Task<REQ,RESP>>(connfd, scheduler, this->m_read_len,this->m_conn_timeout);
            task->set_callback(std::forward<std::function<Task<>(Task_Base::wptr)>>(this->m_func));
            return task;
        }

    protected:
        int m_fd;
        std::vector<std::weak_ptr<Scheduler_Base>> m_schedulers;
        uint32_t m_read_len;
        std::function<Task<>(Task_Base::wptr)> m_func;
        int m_conn_timeout;
        // keepalive
        Tcp_Keepalive_Config m_keepalive_conf;
    };

    template <Tcp_Request REQ, Tcp_Response RESP>
    class Tcp_SSL_RW_Task : public Tcp_RW_Task<REQ,RESP>
    {
    public:
        using ptr = std::shared_ptr<Tcp_SSL_RW_Task>;
        Tcp_SSL_RW_Task(int fd, std::weak_ptr<Scheduler_Base> scheduler, uint32_t read_len,int conn_timeout, SSL *ssl)
            : Tcp_RW_Task<REQ,RESP>(fd, scheduler, read_len,conn_timeout), m_ssl(ssl)
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
            switch (this->m_req->proto_type())
            {
                //buffer read empty
            case GY_ALL_RECIEVE_PROTOCOL_TYPE:
                return read_all_buffer();
                //fixed proto package
            case GY_PACKAGE_FIXED_PROTOCOL_TYPE:
                return read_fixed_pakage();
                //fixed head and head includes body's length
            case GY_HEAD_FIXED_PROTOCOL_TYPE:
                return read_fixed_head();
            }
            return -1;
        }

        int send_package() override
        {
            int len;
            if(this->m_wbuffer.empty()) return 0;
            do{
                len = iofunction::Tcp_Function::SSL_Send(this->m_ssl, this->m_wbuffer, this->m_wbuffer.length());
                if (len != -1 && len != 0) this->m_wbuffer.erase(this->m_wbuffer.begin(), this->m_wbuffer.begin() + len);
                if(this->m_wbuffer.empty()) break;
            }while(len != 0 && len != -1);
            if (len == -1)
            {
                if (errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN)
                {
                    Task_Base::destory();
                    return -1;
                }
            }
            else if (len == 0)
            {
                Task_Base::destory();
                return -1;
            } 
            return 0;
        }

        int read_all_buffer()
        {
            int len;
            do
            {
                memset(this->m_temp, 0, this->m_read_len);
                len = iofunction::Tcp_Function::SSL_Recv(this->m_ssl, this->m_temp, this->m_read_len);
                if (len != -1 && len != 0)
                    this->m_rbuffer.append(this->m_temp, len);
            } while (len != -1 && len != 0);
            if (len == -1)
            {
                if (errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN)
                {
                    Task_Base::destory();
                    return -1;
                }
            }
            else if (len == 0)
            {
                Task_Base::destory();
                return -1;
            }
            int status;
            len = this->m_req->decode(this->m_rbuffer, status);
            if (status == error::protocol_error::GY_PROTOCOL_INCOMPLETE)
                return -1;
            this->m_rbuffer.erase(this->m_rbuffer.begin(), this->m_rbuffer.begin() + len);
            return 0;
        }

        int read_fixed_pakage()
        {
            uint32_t remain_len = this->m_req->proto_fixed_len();
            int len ;
            do{
                memset(this->m_temp, 0, this->m_read_len);
                len = iofunction::Tcp_Function::SSL_Recv(this->m_ssl, this->m_temp, std::min(remain_len,this->m_read_len));
                if (len != -1 && len != 0)
                    this->m_rbuffer.append(this->m_temp, len);
                remain_len -= len;
            }while(len != -1 && len != 0 && remain_len > 0);
            if (len == -1)
            {
                if (errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN)
                {
                    Task_Base::destory();
                    return -1;
                }
            }
            else if (len == 0)
            {
                Task_Base::destory();
                return -1;
            }
            int status;
            len = this->m_req->decode(this->m_rbuffer, status);
            if (status == error::protocol_error::GY_PROTOCOL_INCOMPLETE)
                return -1;
            this->m_rbuffer.erase(this->m_rbuffer.begin(), this->m_rbuffer.begin() + len);
            return 0;
        }

        int read_fixed_head()
        {
            uint32_t remain_len = this->m_req->proto_fixed_len() - this->m_rbuffer.length();
            int len = 1;
            do{
                memset(this->m_temp, 0, this->m_read_len);
                len = iofunction::Tcp_Function::SSL_Recv(this->m_ssl, this->m_temp, std::min(remain_len,this->m_read_len));
                if (len != -1 && len != 0)
                    this->m_rbuffer.append(this->m_temp, len);
                remain_len -= len;
            }while(len != -1 && len != 0 && remain_len > 0);
            if (len == -1)
            {
                if (errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN)
                {
                    Task_Base::destory();
                    return -1;
                }
            }
            else if (len == 0)
            {
                Task_Base::destory();
                return -1;
            }
            int status;
            len = this->m_req->decode(this->m_rbuffer, status); //解析head 里面赋值 proto_extra_len , return head len
            remain_len = this->m_req->proto_extra_len() + this->m_req->proto_fixed_len() - this->m_rbuffer.length();
            do{
                memset(this->m_temp, 0, this->m_read_len);
                len = iofunction::Tcp_Function::SSL_Recv(this->m_ssl, this->m_temp, std::min(remain_len,this->m_read_len));
                if (len != -1 && len != 0)
                    this->m_rbuffer.append(this->m_temp, len);
                remain_len -= len;
            }while(len != -1 && len != 0 && remain_len > 0);
            if (len == -1)
            {
                if (errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN)
                {
                    Task_Base::destory();
                    return -1;
                }
            }
            else if (len == 0)
            {
                Task_Base::destory();
                return -1;
            }
            if(remain_len != 0) return -1;
            this->m_req->set_extra_msg(std::move(this->m_rbuffer.substr(this->m_req->proto_fixed_len())));
            this->m_rbuffer.clear();
            return 0;
        }

    protected:
        SSL *m_ssl = nullptr;
    };

    // tcp's ssl task
    template <Tcp_Request REQ, Tcp_Response RESP>
    class Tcp_SSL_Main_Task : public Tcp_Main_Task<REQ,RESP>
    {
    public:
        using ptr = std::shared_ptr<Tcp_SSL_Main_Task>;
        Tcp_SSL_Main_Task(int fd, std::vector<std::weak_ptr<Scheduler_Base>> schedulers, std::function<Task<>(Task_Base::wptr)> &&func,
                            uint32_t read_len, int conn_timeout, uint32_t ssl_accept_max_retry, SSL_CTX *ctx)
            : Tcp_Main_Task<REQ,RESP>(fd, schedulers, std::forward<std::function<Task<>(Task_Base::wptr)>>(func), read_len,conn_timeout),
              m_ctx(ctx), m_ssl_accept_retry(ssl_accept_max_retry)
        {
        }

        int exec() override
        {
            int connfd = iofunction::Tcp_Function::Accept(this->m_fd);
            if (connfd <= 0)
                return -1;
            if (this->m_keepalive_conf.m_keepalive)
            {
                iofunction::Tcp_Function::Sock_Keepalive(connfd, this->m_keepalive_conf.m_idle, this->m_keepalive_conf.m_interval, this->m_keepalive_conf.m_retry);
            }
            SSL *ssl = iofunction::Tcp_Function::SSL_Create_Obj(this->m_ctx, connfd);
            if (ssl == nullptr)
            {
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
                if (ret <= 0)
                {
                    int ssl_err = SSL_get_error(ssl, ret);
                    if (ssl_err == SSL_ERROR_WANT_READ || ssl_err == SSL_ERROR_WANT_WRITE || ssl_err == SSL_ERROR_WANT_ACCEPT)
                    {
                        continue;
                    }
                    else
                    {
                        iofunction::Tcp_Function::SSL_Destory(ssl);
                        close(connfd);
                        return -1;
                    }
                }
            }
            int indx;
            if (this->m_schedulers.size() == 1)
            {
                indx = 0;
            }else{
                indx = connfd % (this->m_schedulers.size() - 1) + 1;
            }

            if (!this->m_schedulers[indx].expired())
            {
                auto task = create_rw_task(connfd, ssl, this->m_schedulers[indx]);
                this->m_schedulers[indx].lock()->add_task({connfd, task});
                if (this->m_schedulers[indx].lock()->add_event(connfd, GY_EVENT_READ | GY_EVENT_EPOLLET | GY_EVENT_ERROR) == -1)
                {
                    std::cout << "add event failed fd = " << connfd << '\n';
                }
            }
            return 0;
        }

        virtual ~Tcp_SSL_Main_Task()
        {
            if (this->m_ctx)
                this->m_ctx = nullptr;
        }

    protected:
        virtual Task_Base::ptr create_rw_task(int connfd, SSL *ssl,Scheduler_Base::wptr scheduler)
        {
            auto task = std::make_shared<Tcp_SSL_RW_Task<REQ,RESP>>(connfd, scheduler, this->m_read_len,this->m_conn_timeout, ssl);
            task->set_callback(std::forward<std::function<Task<>(Task_Base::wptr)>>(this->m_func));
            return task;
        }

    protected:
        SSL_CTX *m_ctx;
        uint32_t m_ssl_accept_retry;
    };

    

    // time task
    class Time_Task : public Task_Base
    {
    public:
        using ptr = std::shared_ptr<Time_Task>;
        Time_Task(std::weak_ptr<Timer_Manager> manager);

        int exec() override;

        ~Time_Task();

    protected:
        std::weak_ptr<Timer_Manager> m_manager;
    };

    // thread_task
    class Thread_Task : public Task_Base
    {
    public:
        using ptr = std::shared_ptr<Thread_Task>;
        Thread_Task(std::function<void()> &&func);

        int exec() override;

        ~Thread_Task();

    private:
        std::function<void()> m_func;
    };
}

#endif