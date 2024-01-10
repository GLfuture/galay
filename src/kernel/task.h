#ifndef GALAY_TASK_H
#define GALAY_TASK_H

#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <chrono>
#include "../config/config.h"
#include "../protocol/tcp.h"
#include "../protocol/http.h"
#include "coroutine.h"
#include "error.h"
#include "iofunction.h"
#include "scheduler.h"



namespace galay
{
    enum Task_Status
    {
        GY_TASK_CONNECT,
        GY_TASK_READ,
        GY_TASK_WRITE,
        GY_TASK_DISCONNECT,
    };

    class IO_Scheduler;

    class Task_Base
    {
    public:
        using wptr = std::weak_ptr<Task_Base>;
        using ptr = std::shared_ptr<Task_Base>;

        // return -1 error 0 success
        virtual int exec() = 0;

        virtual std::shared_ptr<IO_Scheduler> get_scheduler() {
            return nullptr;
        }

        virtual Request_Base::ptr get_req(){
            return nullptr;
        }
        virtual Response_Base::ptr get_resp(){
            return nullptr;
        }

        virtual void control_task_behavior(Task_Status status){}

        virtual int get_state() {return this->m_status;}

        virtual int get_error() {return this->m_error;}
        
        virtual void finish(){ this->m_is_finish = true;}

        virtual bool is_need_to_destroy() = 0;

        virtual ~Task_Base() {}

    protected:
        int m_status;
        int m_error;
        bool m_is_finish = false;
    };

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
    class Tcp_RW_Task : public Task_Base, public std::enable_shared_from_this<Tcp_RW_Task>
    {
    protected:
        using Callback = std::function<Task<>(Task_Base::wptr)>;

    public:
        using ptr = std::shared_ptr<Tcp_RW_Task>;
        Tcp_RW_Task(int fd, std::weak_ptr<IO_Scheduler> scheduler, uint32_t read_len);

        void control_task_behavior(Task_Status status) override;
        Request_Base::ptr get_req() override { return this->m_req; }
        Response_Base::ptr get_resp() override { return this->m_resp; }

        // return -1 to delete this task from server
        int exec() override;

        std::shared_ptr<IO_Scheduler> get_scheduler() override;

        bool is_need_to_destroy() override;

        std::string &Get_Rbuffer() { return m_rbuffer; }

        std::string &Get_Wbuffer() { return m_wbuffer; }

        void set_callback(Callback &&func) {this->m_func = func;}

        void reset_buffer(int len);

        virtual ~Tcp_RW_Task();

    protected:
        
        int decode();

        void encode();

        virtual int read_package();

        virtual int send_package();
    protected:
        char *m_temp = nullptr;
        std::weak_ptr<IO_Scheduler> m_scheduler;
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
        Tcp_Accept_Task(int fd, std::weak_ptr<IO_Scheduler> scheduler,
                        std::function<Task<>(Task_Base::wptr)> &&func, uint32_t read_len);
        //
        int exec() override;

        bool is_need_to_destroy() override;

        virtual ~Tcp_Accept_Task()
        {
        }

    protected:
        virtual Task_Base::ptr create_rw_task(int connfd);

        virtual void add_task(int connfd, Task_Base::ptr task);
    protected:
        int m_fd;
        std::weak_ptr<IO_Scheduler> m_scheduler;
        uint32_t m_read_len;
        std::function<Task<>(Task_Base::wptr)> m_func;
    };

    class Tcp_SSL_RW_Task : public Tcp_RW_Task
    {
    public:
        using ptr = std::shared_ptr<Tcp_SSL_RW_Task>;
        Tcp_SSL_RW_Task(int fd, std::weak_ptr<IO_Scheduler> scheduler, uint32_t read_len, SSL *ssl)
            : Tcp_RW_Task(fd, scheduler, read_len), m_ssl(ssl)
        {}

        virtual ~Tcp_SSL_RW_Task();

    protected:
        int read_package() override;

        int send_package() override;
    protected:
        SSL *m_ssl = nullptr;
    };

    // tcp's ssl task
    class Tcp_SSL_Accept_Task : public Tcp_Accept_Task
    {
    public:
        using ptr = std::shared_ptr<Tcp_SSL_Accept_Task>;
        Tcp_SSL_Accept_Task(int fd, std::weak_ptr<IO_Scheduler> scheduler, std::function<Task<>(Task_Base::wptr)> &&func, 
                    uint32_t read_len, uint32_t ssl_accept_max_retry, SSL_CTX *ctx) 
                        : Tcp_Accept_Task(fd,scheduler, std::forward<std::function<Task<>(Task_Base::wptr)>>(func), read_len),
                            m_ctx(ctx),m_ssl_accept_retry(ssl_accept_max_retry){}

        int exec() override;

        virtual ~Tcp_SSL_Accept_Task();

    protected:
        virtual Task_Base::ptr create_rw_task(int connfd, SSL *ssl);
    protected:
        SSL_CTX *m_ctx;
        uint32_t m_ssl_accept_retry;
    };

    class Http_RW_Task : public Tcp_RW_Task
    {
    public:
        using ptr = std::shared_ptr<Http_RW_Task>;
        Http_RW_Task(int fd, std::weak_ptr<IO_Scheduler> scheduler, uint32_t read_len) 
            : Tcp_RW_Task(fd, scheduler, read_len)
        {
            this->m_req = std::make_shared<Http_Request>();
            this->m_resp = std::make_shared<Http_Response>();
        }
        
        bool is_need_to_destroy() override;

        int exec() override;
        
        ~Http_RW_Task()
        {
        }
    private:
        bool send_head = true;
        bool m_is_chunked = false;
    };

    class Http_Accept_Task : public Tcp_Accept_Task
    {
    public:
        using ptr = std::shared_ptr<Http_Accept_Task>;
        Http_Accept_Task(int fd, std::weak_ptr<IO_Scheduler> scheduler, std::function<Task<>(Task_Base::wptr)> &&func, uint32_t read_len) 
                        : Tcp_Accept_Task(fd, scheduler, std::forward<std::function<Task<>(Task_Base::wptr)>>(func), read_len)
        {}

        Task_Base::ptr create_rw_task(int connfd) override;
    };

    class Https_RW_Task : public Tcp_SSL_RW_Task
    {
    public:
        using ptr = std::shared_ptr<Https_RW_Task>;
        Https_RW_Task(int fd, std::weak_ptr<IO_Scheduler> scheduler, uint32_t read_len, SSL *ssl)
            : Tcp_SSL_RW_Task(fd, scheduler, read_len, ssl)
        {
            this->m_req = std::make_shared<Http_Request>();
            this->m_resp = std::make_shared<Http_Response>();
        }


        int exec() override;

        virtual ~Https_RW_Task() {}
    private:
        bool send_head = true;
        bool m_is_chunked = false;
    };


    // https task
    class Https_Accept_Task : public Tcp_SSL_Accept_Task
    {
    public:
        using ptr = std::shared_ptr<Https_Accept_Task>;
        Https_Accept_Task(int fd, std::weak_ptr<IO_Scheduler> scheduler, std::function<Task<>(Task_Base::wptr)> &&func
                    , uint32_t read_len, uint32_t ssl_accept_max_retry, SSL_CTX *ctx) 
                        : Tcp_SSL_Accept_Task(fd,scheduler, std::forward<std::function<Task<>(Task_Base::wptr)>>(func), read_len, ssl_accept_max_retry, ctx) {}

    private:
        Task_Base::ptr create_rw_task(int connfd, SSL *ssl) override;
    };

}

#endif