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
        Server(Config::ptr config , IO_Scheduler::ptr scheduler) 
            : m_config(config),m_scheduler(scheduler){}

        virtual void start(std::function<Task<>(std::weak_ptr<Task_Base>)> &&func) = 0;

        virtual int get_error() { return this->m_error; }
    
        virtual void stop(){ this->m_scheduler->stop(); }

        virtual IO_Scheduler::ptr get_scheduler() { return this->m_scheduler; }

        virtual ~Server();

    protected:
        int m_fd = 0;
        Config::ptr m_config;
        int m_error = error::base_error::GY_SUCCESS;
        IO_Scheduler::ptr m_scheduler;
    };

    //tcp_server
    class Tcp_Server : public Server
    {
    public:
        using ptr = std::shared_ptr<Tcp_Server>;
        Tcp_Server() = delete;
        Tcp_Server(Tcp_Server_Config::ptr config , IO_Scheduler::ptr scheduler) 
            : Server(config,scheduler){}

        void start(std::function<Task<>(Task_Base::wptr)> &&func) override;

        virtual ~Tcp_Server(){}
    protected:
        int init(Tcp_Server_Config::ptr config);

        virtual void add_accept_task(std::function<Task<>(Task_Base::wptr)> &&func , uint32_t recv_len);
    };

    
    class Tcp_SSL_Server: public Tcp_Server
    {
    public:
        Tcp_SSL_Server(Tcp_SSL_Server_Config::ptr config, IO_Scheduler::ptr scheduler);

        ~Tcp_SSL_Server() override;

    protected:
        void add_accept_task(std::function<Task<>(Task_Base::wptr)> &&func , uint32_t recv_len) override;
    protected:
        SSL_CTX *m_ctx = nullptr;
    };

    class Http_Server : public Tcp_Server
    {
    public:
        using ptr = std::shared_ptr<Http_Server>;
        Http_Server(Http_Server_Config::ptr config, IO_Scheduler::ptr scheduler):
            Tcp_Server(config,scheduler){}

    protected:
        void add_accept_task(std::function<Task<>(Task_Base::wptr)> &&func , uint32_t max_recv_len) override;
    };


    class Https_Server: public Tcp_SSL_Server
    {
    public:
        using ptr = std::shared_ptr<Https_Server>;
        Https_Server(Https_Server_Config::ptr config , IO_Scheduler::ptr scheduler)
            :Tcp_SSL_Server(config,scheduler){}
        
    protected:
        //添加accept task
        void add_accept_task(std::function<Task<>(Task_Base::wptr)> &&func , uint32_t max_recv_len) override;
        
    };

    
    // to do
    class Udp_Server : public Server
    {
    public:
    };

}

#endif