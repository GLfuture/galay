#ifndef GALAY_SERVER_HPP
#define GALAY_SERVER_HPP

#include "../kernel/task.h"
#include <functional>
#include <atomic>
#include <type_traits>

namespace galay
{
    
    class Server
    {
    public:
        Server(Config::ptr config , Scheduler_Base::ptr scheduler) 
            : m_config(config),m_scheduler(scheduler){}

        virtual void start(std::function<Task<>(std::weak_ptr<Task_Base>)> &&func) = 0;

        virtual int get_error() { return this->m_error; }
    
        virtual void stop(){ this->m_scheduler->stop(); }

        virtual Scheduler_Base::ptr get_scheduler() { return this->m_scheduler; }

        virtual ~Server();

    protected:
        int m_fd = 0;
        Config::ptr m_config;
        int m_error = error::base_error::GY_SUCCESS;
        Scheduler_Base::ptr m_scheduler;
    };

    //tcp_server
    template<Request REQ,Response RESP>
    class Tcp_Server : public Server
    {
    public:
        using ptr = std::shared_ptr<Tcp_Server>;
        Tcp_Server() = delete;
        Tcp_Server(Tcp_Server_Config::ptr config , Scheduler_Base::ptr scheduler) 
            : Server(config,scheduler){}

        void start(std::function<Task<>(Task_Base::wptr)> &&func) override
        {
            Tcp_Server_Config::ptr config = std::dynamic_pointer_cast<Tcp_Server_Config>(this->m_config);
            this->m_error = init(config);
            if (this->m_error != error::base_error::GY_SUCCESS)
                return;
            add_main_task(std::forward<std::function<Task<>(Task_Base::wptr)>>(func), config->m_max_rbuffer_len);
            this->m_error = this->m_scheduler->start();
        }

        virtual ~Tcp_Server(){}
    protected:
        int init(Tcp_Server_Config::ptr config)
        {
            this->m_fd = iofunction::Tcp_Function::Sock();
            if (this->m_fd <= 0)
            {
                return error::base_error::GY_SOCKET_ERROR;
            }
            int ret = iofunction::Tcp_Function::Reuse_Fd(this->m_fd);
            if (ret == -1)
            {
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

        virtual void add_main_task(std::function<Task<>(Task_Base::wptr)> &&func, uint32_t recv_len)
        {
            if(this->m_scheduler->add_event(this->m_fd, GY_EVENT_READ | GY_EVENT_EPOLLET| GY_EVENT_ERROR)==-1)
            {
                std::cout<< "add event failed fd = " <<this->m_fd <<'\n';
            }
            auto task = std::make_shared<Tcp_Main_Task<REQ,RESP>>(this->m_fd, this->m_scheduler, std::forward<std::function<Task<>(Task_Base::wptr)>>(func), recv_len);
            auto config = std::dynamic_pointer_cast<Tcp_Server_Config>(this->m_config);
            if (config->m_keepalive)
            {
                task->enable_keepalive(config->m_idle, config->m_interval, config->m_retry);
            }
            this->m_scheduler->add_task(std::make_pair(this->m_fd, task));
        }
    };

    template<Request REQ,Response RESP>
    class Tcp_SSL_Server: public Tcp_Server<REQ,RESP>
    {
    public:
        Tcp_SSL_Server(Tcp_SSL_Server_Config::ptr config, Scheduler_Base::ptr scheduler)
            : Tcp_Server<REQ,RESP>(config, scheduler)
        {
            m_ctx = iofunction::Tcp_Function::SSL_Init_Server(config->m_ssl_min_version, config->m_ssl_max_version);
            if (m_ctx == nullptr)
            {
                this->m_error = error::server_error::GY_SSL_CTX_INIT_ERROR;
                ERR_print_errors_fp(stderr);
                exit(-1);
            }
            if (iofunction::Tcp_Function::SSL_Config_Cert_And_Key(m_ctx, config->m_cert_filepath.c_str(), config->m_key_filepath.c_str()) == -1)
            {
                this->m_error = error::server_error::GY_SSL_CRT_OR_KEY_FILE_ERROR;
                ERR_print_errors_fp(stderr);
                exit(-1);
            }
        }

        ~Tcp_SSL_Server() override
        {
            if (this->m_ctx)
            {
                iofunction::Tcp_Function::SSL_Destory({}, m_ctx);
                this->m_ctx = nullptr;
            }
        }

    protected:
        void add_main_task(std::function<Task<>(Task_Base::wptr)> &&func, uint32_t recv_len) override
        {
            if(this->m_scheduler->add_event(this->m_fd, GY_EVENT_READ | GY_EVENT_EPOLLET| GY_EVENT_ERROR)==-1)
            {
                std::cout<< "add event failed fd = " <<this->m_fd <<'\n';
            }
            Tcp_SSL_Server_Config::ptr config = std::dynamic_pointer_cast<Tcp_SSL_Server_Config>(this->m_config);
            auto task = std::make_shared<Tcp_SSL_Main_Task<REQ,RESP>>(this->m_fd, this->m_scheduler, std::forward<std::function<Task<>(Task_Base::wptr)>>(func), recv_len, config->m_ssl_accept_retry, this->m_ctx);
            if (config->m_keepalive)
            {
                task->enable_keepalive(config->m_idle, config->m_interval, config->m_retry);
            }
            this->m_scheduler->add_task(std::make_pair(this->m_fd, task));
        }

    protected:
        SSL_CTX *m_ctx = nullptr;
    };

    // to do
    class Udp_Server : public Server
    {
    public:
    };

}

#endif