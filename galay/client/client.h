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

        void stop(){
            if (!this->m_scheduler.expired() && !this->m_stop)
            {
                this->m_scheduler.lock()->del_task(this->m_fd);
                this->m_stop = true;
            }
        }

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
        using ptr = std::shared_ptr<Tcp_Client>;
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
        using ptr = std::shared_ptr<Tcp_SSL_Client>;
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

    class Http_Client: public Tcp_Client
    {
    public:
        using ptr = std::shared_ptr<Http_Client>;
        Http_Client(Scheduler_Base::wptr scheduler)
            :Tcp_Client(scheduler) {}


        Net_Awaiter<int> request(Http_Request::ptr request,Http_Response::ptr response);

    private:
        Net_Awaiter<int> send(const std::string &buffer,uint32_t len) override { return {nullptr} ;}
        Net_Awaiter<int> recv(char* buffer,int len) override { return {nullptr} ;}
    };


    class Https_Client: public Tcp_SSL_Client
    {
    public:
        using ptr = std::shared_ptr<Https_Client>;
        Https_Client(Scheduler_Base::wptr scheduler, long ssl_min_version , long ssl_max_version)
            :Tcp_SSL_Client(scheduler, ssl_min_version ,ssl_max_version){}
        Net_Awaiter<int> request(Http_Request::ptr request,Http_Response::ptr response);
    private:
        Net_Awaiter<int> send(const std::string &buffer,uint32_t len) override { return {nullptr} ;}
        Net_Awaiter<int> recv(char* buffer,int len) override { return {nullptr} ;}
    };

}


#endif