#ifndef GALAY_TASK_H
#define GALAY_TASK_H

#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <chrono>
#include "iofunction.h"
#include "basic_concepts.h"
#include "error.h"
#include "engine.h"


namespace galay
{
    enum task_state
    {
        GY_TASK_STOP,
        GY_TASK_READ,
        GY_TASK_WRITE,
    };

    template <Request REQ, Response RESP>
    class Server;

    template <Request REQ, Response RESP>
    class Tcp_RW_Task;

    template <Request REQ, Response RESP>
    class Tcp_SSL_RW_Task;

    template <Request REQ, Response RESP>
    class Http_RW_Task;
    
    template <Request REQ, Response RESP>
    class Https_RW_Task;

    template <Request REQ, Response RESP>
    class Task
    {
    public:
        using ptr = std::shared_ptr<Task>;
        virtual std::shared_ptr<REQ> get_req() = 0;
        virtual std::shared_ptr<RESP> get_resp() = 0;

        // return -1 error 0 success
        virtual int exec() = 0;
        virtual int get_state()
        {
            return this->m_state;
        }

        virtual int get_error()
        {
            return this->m_error;
        }

        virtual ~Task() {}

    protected:
        int m_state;
        int m_error;
    };

    // tcp server accept task
    template <Request REQ, Response RESP>
    class Tcp_Accept_Task : public Task<REQ, RESP>
    {
    public:
        using ptr = std::shared_ptr<Tcp_Accept_Task>;
        Tcp_Accept_Task(int fd, Engine::ptr engine, std::unordered_map<int, std::shared_ptr<Task<REQ, RESP>>> *tasks,
                        std::function<void(std::shared_ptr<Task<REQ, RESP>>)> &&func, uint32_t read_len)
        {
            this->m_fd = fd;
            this->m_engine = engine;
            this->m_state = task_state::GY_TASK_READ;
            this->m_tasks = tasks;
            this->m_error = error::base_error::GY_SUCCESS;
            this->m_func = func;
            this->m_read_len = read_len;
        }

        std::shared_ptr<RESP> get_resp() override
        {
            return nullptr;
        }
        std::shared_ptr<REQ> get_req() override
        {
            return nullptr;
        };

        //
        int exec() override
        {
            int connfd = iofunction::Tcp_Function::Accept(this->m_fd);
            if (connfd <= 0)
                return -1;
            add_rw_task(connfd);
            iofunction::Tcp_Function::IO_Set_No_Block(connfd);
            m_engine->add_event(connfd, EPOLLIN | EPOLLET);
            return 0;
        }

        virtual ~Tcp_Accept_Task()
        {
        }

    protected:
        virtual void add_rw_task(int connfd)
        {
            auto it = this->m_tasks->find(connfd);
            if (it != this->m_tasks->end())
            {
                it->second.reset(new Tcp_RW_Task<REQ, RESP>(connfd, m_engine, this->m_read_len));
                auto task = std::static_pointer_cast<Tcp_RW_Task<REQ, RESP>>(it->second);
                task->set_callback(std::forward<std::function<void(std::shared_ptr<Task<REQ, RESP>>)>>(this->m_func));
            }
            else
            {
                auto task = std::make_shared<Tcp_RW_Task<REQ, RESP>>(connfd, m_engine, this->m_read_len);
                task->set_callback(std::forward<std::function<void(std::shared_ptr<Task<REQ, RESP>>)>>(this->m_func));
                this->m_tasks->emplace(connfd, task);
            }
        }

    protected:
        int m_fd;
        Engine::ptr m_engine;
        uint32_t m_read_len;
        std::unordered_map<int, std::shared_ptr<Task<REQ, RESP>>> *m_tasks;
        std::function<void(std::shared_ptr<Task<REQ, RESP>>)> m_func;
    };

    // tcp
    // server read and write task
    template <Request REQ, Response RESP>
    class Tcp_RW_Task : public Task<REQ, RESP>, public std::enable_shared_from_this<Tcp_RW_Task<REQ, RESP>>
    {
    protected:
        using Callback = std::function<void(std::shared_ptr<Task<REQ, RESP>>)>;

    public:
        using ptr = std::shared_ptr<Tcp_RW_Task>;
        Tcp_RW_Task(int fd, Engine::ptr engine, uint32_t read_len)
        {
            this->m_req = std::make_shared<REQ>();
            this->m_resp = std::make_shared<RESP>();
            this->m_state = task_state::GY_TASK_READ;
            this->m_error = error::base_error::GY_SUCCESS;
            this->m_engine = engine;
            this->m_fd = fd;
            this->m_read_len = read_len;
            this->m_temp = new char[read_len];
        }

        std::string &Get_Rbuffer() { return m_rbuffer; }

        std::string &Get_Wbuffer() { return m_wbuffer; }

        std::shared_ptr<REQ> get_req() override { return this->m_req; }
        std::shared_ptr<RESP> get_resp() override { return this->m_resp; }

        // return -1 to delete this task from server
        int exec() override
        {
            switch (this->m_state)
            {
            case task_state::GY_TASK_READ:
            {
                if (read_package() == -1)
                    return -1;
                if (decode() == error::protocol_error::GY_PROTOCOL_INCOMPLETE)
                    return -1;
                this->m_func(this->shared_from_this());
                encode();
                this->m_engine->mod_event(this->m_fd, EPOLLOUT);
                this->m_state = task_state::GY_TASK_WRITE;
                break;
            }
            case task_state::GY_TASK_WRITE:
            {
                if (send_package() == -1)
                    return -1;
                this->m_engine->mod_event(this->m_fd, EPOLLIN);
                this->m_state = task_state::GY_TASK_READ;
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

        Engine::ptr get_engine()
        {
            return this->m_engine;
        }

        void reset_buffer(int len)
        {
            if (this->m_temp)
                delete[] this->m_temp;
            this->m_temp = new char[len];
            this->m_read_len = len;
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
            m_wbuffer = m_resp->encode();
        }

        virtual int read_package()
        {
            this->m_error = error::base_error::GY_SUCCESS;
            int len = iofunction::Tcp_Function::Recv(this->m_fd, this->m_temp, this->m_read_len);
            if (len == 0)
            {
                close(this->m_fd);
                this->m_engine->del_event(this->m_fd, EPOLLIN);
                this->m_state = task_state::GY_TASK_STOP;
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
                    this->m_engine->del_event(this->m_fd, EPOLLIN);
                    close(this->m_fd);
                    this->m_state = task_state::GY_TASK_STOP;
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
                    this->m_engine->del_event(this->m_fd, EPOLLIN);
                    close(this->m_fd);
                    this->m_state = task_state::GY_TASK_STOP;
                    return -1;
                }
            }
            this->m_wbuffer.erase(this->m_wbuffer.begin(), this->m_wbuffer.begin() + len);
            return 0;
        }

    protected:
        char *m_temp = nullptr;
        Engine::ptr m_engine;
        int m_fd;
        uint32_t m_read_len;
        std::string m_rbuffer;
        std::string m_wbuffer;
        std::shared_ptr<REQ> m_req;
        std::shared_ptr<RESP> m_resp;
        Callback m_func;
    };
    

    //tcp's ssl task
    template <Request REQ, Response RESP>
    class Tcp_SSL_Accept_Task : public Tcp_Accept_Task<REQ, RESP>
    {
    public:
        using ptr = std::shared_ptr<Tcp_SSL_Accept_Task>;
        Tcp_SSL_Accept_Task(int fd, Engine::ptr engine, std::unordered_map<int, std::shared_ptr<Task<REQ, RESP>>> *tasks,
                            std::function<void(std::shared_ptr<Task<REQ, RESP>>)> &&func, uint32_t read_len , uint32_t ssl_accept_max_retry, SSL_CTX *ctx) : 
                            Tcp_Accept_Task<REQ, RESP>(fd, engine, tasks, std::forward<std::function<void(std::shared_ptr<Task<REQ, RESP>>)>>(func), read_len)
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
            iofunction::Tcp_Function::IO_Set_No_Block(connfd);
            int ret = iofunction::Tcp_Function::SSL_Accept(ssl);
            
            if (ret == 0)
            {
                close(this->m_fd);
                return -1;
            }
            uint32_t retry = 0;
            while(ret == -1 && retry ++ <= this->m_ssl_accept_retry)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_SSL_SLEEP_MISC_PER_RETRY));
                ret = iofunction::Tcp_Function::SSL_Accept(ssl);
            }
            add_rw_task(connfd, ssl);
            this->m_engine->add_event(connfd, EPOLLIN | EPOLLET);
            return 0;
        }

        virtual ~Tcp_SSL_Accept_Task()
        {
            if (this->m_ctx)
                this->m_ctx = nullptr;
        }

    protected:
        virtual void add_rw_task(int connfd, SSL *ssl)
        {
            auto it = this->m_tasks->find(connfd);
            if (it != this->m_tasks->end())
            {
                it->second.reset(new Tcp_SSL_RW_Task<REQ, RESP>(connfd, this->m_engine, this->m_read_len, ssl));
                auto task = std::static_pointer_cast<Tcp_SSL_RW_Task<REQ, RESP>>(it->second);
                task->set_callback(std::forward<std::function<void(std::shared_ptr<Task<REQ, RESP>>)>>(this->m_func));
            }
            else
            {
                auto task = std::make_shared<Tcp_SSL_RW_Task<REQ, RESP>>(connfd, this->m_engine, this->m_read_len, ssl);
                task->set_callback(std::forward<std::function<void(std::shared_ptr<Task<REQ, RESP>>)>>(this->m_func));
                this->m_tasks->emplace(connfd, task);
            }
        }

    protected:
        SSL_CTX *m_ctx;
        uint32_t m_ssl_accept_retry;
    };

    template <Request REQ, Response RESP>
    class Tcp_SSL_RW_Task : public Tcp_RW_Task<REQ, RESP>
    {
    public:
        using ptr = std::shared_ptr<Tcp_SSL_RW_Task>;
        Tcp_SSL_RW_Task(int fd, Engine::ptr engine, uint32_t read_len, SSL *ssl) 
            : Tcp_RW_Task<REQ, RESP>(fd, engine, read_len), m_ssl(ssl)
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
                close(this->m_fd);
                this->m_engine->del_event(this->m_fd, EPOLLIN);
                this->m_state = task_state::GY_TASK_STOP;
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
                    this->m_engine->del_event(this->m_fd, EPOLLIN);
                    close(this->m_fd);
                    this->m_state = task_state::GY_TASK_STOP;
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
                    this->m_engine->del_event(this->m_fd, EPOLLIN);
                    close(this->m_fd);
                    this->m_state = task_state::GY_TASK_STOP;
                    return -1;
                }
            }
            this->m_wbuffer.erase(this->m_wbuffer.begin(), this->m_wbuffer.begin() + len);
            return 0;
        }

    protected:
        SSL *m_ssl = nullptr;
    };

    // http server's task
    template <Request REQ, Response RESP>
    class Http_Accept_Task : public Tcp_Accept_Task<REQ, RESP>
    {
    public:
        using ptr = std::shared_ptr<Http_Accept_Task>;
        Http_Accept_Task(int fd, Engine::ptr engine, std::unordered_map<int, std::shared_ptr<Task<REQ, RESP>>> *tasks,
                         std::function<void(std::shared_ptr<Task<REQ, RESP>>)> &&func, uint32_t read_len) : Tcp_Accept_Task<REQ, RESP>(fd, engine, tasks, std::forward<std::function<void(std::shared_ptr<Task<REQ, RESP>>)>>(func), read_len)
        {
        }

        void add_rw_task(int connfd)
        {
            auto it = this->m_tasks->find(connfd);
            if (it != this->m_tasks->end())
            {
                it->second.reset(new Http_RW_Task<REQ, RESP>(connfd, this->m_engine, this->m_read_len));
                auto task = std::static_pointer_cast<Http_RW_Task<REQ, RESP>>(it->second);
                task->set_callback(std::forward<std::function<void(std::shared_ptr<Task<REQ, RESP>>)>>(this->m_func));
            }
            else
            {
                auto task = std::make_shared<Http_RW_Task<REQ, RESP>>(connfd, this->m_engine, this->m_read_len);
                task->set_callback(std::forward<std::function<void(std::shared_ptr<Task<REQ, RESP>>)>>(this->m_func));
                this->m_tasks->emplace(connfd, task);
            }
        }
    };

    template <Request REQ, Response RESP>
    class Http_RW_Task : public Tcp_RW_Task<REQ, RESP>
    {
    public:
        using ptr = std::shared_ptr<Http_RW_Task>;
        Http_RW_Task(int fd, Engine::ptr engine, uint32_t read_len) : Tcp_RW_Task<REQ, RESP>(fd, engine, read_len)
        {
        }

        int exec() override
        {
            switch (this->m_state)
            {
            case task_state::GY_TASK_READ:
            {
                if (Tcp_RW_Task<REQ, RESP>::read_package() == -1)
                    return -1;
                if (Tcp_RW_Task<REQ, RESP>::decode() == error::protocol_error::GY_PROTOCOL_INCOMPLETE)
                    return -1;
                this->m_func(this->shared_from_this());
                Tcp_RW_Task<REQ, RESP>::encode();
                this->m_engine->mod_event(this->m_fd, EPOLLOUT);
                this->m_state = task_state::GY_TASK_WRITE;
                break;
            }
            case task_state::GY_TASK_WRITE:
            {
                if (Tcp_RW_Task<REQ, RESP>::send_package() == -1)
                    return -1;
                this->m_engine->del_event(this->m_fd, EPOLLIN);
                close(this->m_fd);
                this->m_state = task_state::GY_TASK_STOP;
                break;
            }
            default:
                break;
            }
            return 0;
        }
    };

    //https task
    template <Request REQ, Response RESP>
    class Https_Accept_Task : public Tcp_SSL_Accept_Task<REQ, RESP>
    {
    public:
        using ptr = std::shared_ptr<Https_Accept_Task>;
        Https_Accept_Task(int fd, Engine::ptr engine, std::unordered_map<int, std::shared_ptr<Task<REQ, RESP>>> *tasks,
                            std::function<void(std::shared_ptr<Task<REQ, RESP>>)> &&func, uint32_t read_len , uint32_t ssl_accept_max_retry, SSL_CTX *ctx) : 
                            Tcp_SSL_Accept_Task<REQ, RESP>(fd, engine, tasks, std::forward<std::function<void(std::shared_ptr<Task<REQ, RESP>>)>>(func)
                                , read_len , ssl_accept_max_retry , ctx){}
        
        void add_rw_task(int connfd, SSL *ssl) override
        {
            auto it = this->m_tasks->find(connfd);
            if (it != this->m_tasks->end())
            {
                it->second.reset(new Https_RW_Task<REQ, RESP>(connfd, this->m_engine, this->m_read_len , ssl));
                auto task = std::static_pointer_cast<Https_RW_Task<REQ, RESP>>(it->second);
                task->set_callback(std::forward<std::function<void(std::shared_ptr<Task<REQ, RESP>>)>>(this->m_func));
            }
            else
            {
                auto task = std::make_shared<Https_RW_Task<REQ, RESP>>(connfd, this->m_engine, this->m_read_len, ssl);
                task->set_callback(std::forward<std::function<void(std::shared_ptr<Task<REQ, RESP>>)>>(this->m_func));
                this->m_tasks->emplace(connfd, task);
            }
        }


    };

    template <Request REQ, Response RESP>
    class Https_RW_Task : public Tcp_SSL_RW_Task<REQ, RESP>
    {
    public:
        using ptr = std::shared_ptr<Https_RW_Task>;
        Https_RW_Task(int fd, Engine::ptr engine, uint32_t read_len, SSL *ssl)
            :Tcp_SSL_RW_Task<REQ,RESP>(fd,engine,read_len,ssl)
        {

        }

        int exec() override
        {
            switch (this->m_state)
            {
            case task_state::GY_TASK_READ:
            {
                if (Tcp_SSL_RW_Task<REQ, RESP>::read_package() == -1)
                    return -1;
                if (Tcp_SSL_RW_Task<REQ, RESP>::decode() == error::protocol_error::GY_PROTOCOL_INCOMPLETE)
                    return -1;
                this->m_func(this->shared_from_this());
                Tcp_SSL_RW_Task<REQ, RESP>::encode();
                this->m_engine->mod_event(this->m_fd, EPOLLOUT);
                this->m_state = task_state::GY_TASK_WRITE;
                break;
            }
            case task_state::GY_TASK_WRITE:
            {
                if (Tcp_SSL_RW_Task<REQ, RESP>::send_package() == -1)
                    return -1;
                this->m_engine->del_event(this->m_fd, EPOLLIN);
                close(this->m_fd);
                this->m_state = task_state::GY_TASK_STOP;
                break;
            }
            default:
                break;
            }
            return 0;
        }
        
    };

}

#endif