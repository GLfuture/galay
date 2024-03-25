#ifndef GALAY_CLIENT_H
#define GALAY_CLIENT_H

#include "../config/config.h"
#include "../kernel/basic_concepts.h"
#include "../kernel/scheduler.h"
#include "../kernel/awaiter.h"

namespace galay
{
    class Client
    {
    public:
        Client(Scheduler_Base::wptr scheduler)
            :m_scheduler(scheduler){}

        int get_error() { return this->m_error; }

        void stop();

        virtual ~Client();

    protected:
        bool m_stop = false;
        int m_fd;
        int m_error;
        Scheduler_Base::wptr m_scheduler;
    };

    class Tcp_Client: public Client
    {
    public:
        using uptr = std::unique_ptr<Tcp_Client>;
        //创建时自动sock
        Tcp_Client(Scheduler_Base::wptr scheduler);

        //0 is success  -1 is failed
        virtual Net_Awaiter<int> connect(std::string ip , uint32_t port);

        //-1 is failed >0 is successful
        //if >0 return the value that send's length
        virtual Net_Awaiter<int> send(const std::string &buffer,uint32_t len);
        
        //-1 is failed >0 is success 
        //if >0 return the value that recv's length
        virtual Net_Awaiter<int> recv(char* buffer,int len);

        virtual ~Tcp_Client();
    };

    class Tcp_SSL_Client: public Tcp_Client
    {
    public:
        using uptr = std::unique_ptr<Tcp_SSL_Client>;
        Tcp_SSL_Client(Scheduler_Base::wptr scheduler, long ssl_min_version , long ssl_max_version);
        
        //0 is success  -1 is failed
        virtual Net_Awaiter<int> connect(std::string ip , uint32_t port);

        //-1 is failed >0 is successful
        //if >0 return the value that send's length
        virtual Net_Awaiter<int> send(const std::string &buffer,uint32_t len);
        
        //-1 is failed >0 is success 
        //if >0 return the value that recv's length
        virtual Net_Awaiter<int> recv(char* buffer,int len);


        virtual ~Tcp_SSL_Client();
    protected:
        SSL* m_ssl;
        SSL_CTX* m_ctx;
    };

    class Tcp_Request_Client: public Tcp_Client
    {
    public:
        using uptr = std::unique_ptr<Tcp_Request_Client>;
        Tcp_Request_Client(Scheduler_Base::wptr scheduler)
            :Tcp_Client(scheduler) {}
        
        template<Tcp_Request REQ,Tcp_Response RESP>
        Net_Awaiter<int> request(std::shared_ptr<REQ> request, std::shared_ptr<RESP> response)
        {
            if (!this->m_scheduler.expired())
            {
                typename Co_Tcp_Client_Request_Task<REQ,RESP,int>::ptr task = std::make_shared<Co_Tcp_Client_Request_Task<REQ,RESP,int>>(this->m_fd, this->m_scheduler, request, response, &(this->m_error));
                if (this->m_scheduler.lock()->add_event(this->m_fd, GY_EVENT_WRITE | GY_EVENT_EPOLLET | GY_EVENT_ERROR) == -1)
                {
                    std::cout << "add event failed fd = " << this->m_fd << '\n';
                }
                this->m_scheduler.lock()->add_task({this->m_fd, task});
                return Net_Awaiter<int>{task};
            }
            this->m_error = Error::SchedulerError::GY_SCHDULER_EXPIRED_ERROR;
            return {nullptr, -1};
        }

    private:
        Net_Awaiter<int> send(const std::string &buffer,uint32_t len) override { return {nullptr} ;}
        Net_Awaiter<int> recv(char* buffer,int len) override { return {nullptr} ;}
    };


    class Tcp_SSL_Request_Client: public Tcp_SSL_Client
    {
    public:
        using uptr = std::unique_ptr<Tcp_SSL_Request_Client>;
        Tcp_SSL_Request_Client(Scheduler_Base::wptr scheduler, long ssl_min_version, long ssl_max_version)
            : Tcp_SSL_Client(scheduler, ssl_min_version, ssl_max_version) {}

        template<Tcp_Request REQ,Tcp_Response RESP>
        Net_Awaiter<int> request(std::shared_ptr<REQ> request, std::shared_ptr<RESP> response)
        {
            if (!this->m_scheduler.expired())
            {
                auto task = std::make_shared<Co_Tcp_Client_SSL_Request_Task<REQ, RESP, int>>(this->m_ssl, this->m_fd, this->m_scheduler, request, response, &(this->m_error));
                if (this->m_scheduler.lock()->add_event(this->m_fd, GY_EVENT_WRITE | GY_EVENT_EPOLLET | GY_EVENT_ERROR) == -1)
                {
                    std::cout << "add event failed fd = " << this->m_fd << '\n';
                }
                this->m_scheduler.lock()->add_task({this->m_fd, task});
                return Net_Awaiter<int>{task};
            }
            this->m_error = Error::SchedulerError::GY_SCHDULER_EXPIRED_ERROR;
            return {nullptr, -1};
        }

    private:
        Net_Awaiter<int> send(const std::string &buffer,uint32_t len) override { return {nullptr} ;}
        Net_Awaiter<int> recv(char* buffer,int len) override { return {nullptr} ;}
    };


    //任务内置定时器,默认最大超时时间为 5000ms
    class Udp_Client: public Client
    {
    public:
        using uptr = std::unique_ptr<Udp_Client>;
        Udp_Client(Scheduler_Base::wptr scheduler);

        virtual Net_Awaiter<int> sendto(std::string ip,uint32_t port,std::string buffer);

        virtual Net_Awaiter<int> recvfrom(char* buffer, int len , IOFuntion::Addr* addr);

    };

    class Udp_Request_Client: public Udp_Client
    {
    public:
        using uptr = std::unique_ptr<Udp_Request_Client>;
        Udp_Request_Client(Scheduler_Base::wptr scheduler)
            : Udp_Client(scheduler)
        {
        }

        template<Udp_Request REQ,Udp_Response RESP>
        Net_Awaiter<int> request(std::shared_ptr<REQ> request, std::shared_ptr<RESP> response, std::string ip, uint32_t port)
        {
            if (!this->m_scheduler.expired())
            {
                auto task = std::make_shared<Co_Dns_Client_Request_Task<REQ,RESP,int>>(this->m_fd, ip, port, request, response, this->m_scheduler, &(this->m_error));
                if (this->m_scheduler.lock()->add_event(this->m_fd, GY_EVENT_WRITE | GY_EVENT_EPOLLET | GY_EVENT_ERROR) == -1)
                {
                    std::cout << "add event failed fd = " << this->m_fd << '\n';
                }
                this->m_scheduler.lock()->add_task({this->m_fd, task});
                return Net_Awaiter<int>{task};
            }
            this->m_error = Error::SchedulerError::GY_SCHDULER_EXPIRED_ERROR;
            return {nullptr, -1};
        }

    private:
        Net_Awaiter<int> sendto(std::string ip,uint32_t port,std::string buffer) override { return {nullptr};  }
        Net_Awaiter<int> recvfrom(char* buffer, int len , IOFuntion::Addr* addr) override { return {nullptr};  }
    };
}


#endif