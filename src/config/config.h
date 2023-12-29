#ifndef GALAY_CONFIG_H
#define GALAY_CONFIG_H

#include <iostream>
#include <memory>
#include <string>
#include <cxxabi.h>
#include <fstream>

namespace galay{
    #define DEFAULT_EVENT_SIZE              2048
    #define DEFAULT_EVENT_TIME_OUT          5
    #define DEFAULT_RECV_LENGTH             1024
    #define DEFAULT_FD_NUM                  2000

    enum IO_ENGINE{
        IO_SELECT,
        IO_POLL,
        IO_EPOLL,
        IO_URING,
    };

    class Config
    {
    public:
        using ptr = std::shared_ptr<Config>;
        virtual ~Config() {}
    };

    //tcp server配置类
    class Tcp_Server_Config: public Config
    {
    public:
        using ptr = std::shared_ptr<Tcp_Server_Config>;
        Tcp_Server_Config(uint16_t port,uint32_t backlog,IO_ENGINE engine ,bool is_ssl = false) : 
            m_port(port),m_backlog(backlog),m_engine(engine) , m_is_ssl(is_ssl) //,Config()
        {

        } 

        Tcp_Server_Config(Tcp_Server_Config&& other)
        {
            this->m_backlog = other.m_backlog;
            this->m_engine = other.m_engine;
            this->m_event_size = other.m_event_size;
            this->m_event_time_out = other.m_event_time_out;
            this->m_is_ssl = other.m_is_ssl;
            this->m_port = other.m_port;
            this->m_recv_len = other.m_recv_len;
        }

        Tcp_Server_Config(const Tcp_Server_Config& other)
        {
            this->m_backlog = other.m_backlog;
            this->m_engine = other.m_engine;
            this->m_event_size = other.m_event_size;
            this->m_event_time_out = other.m_event_time_out;
            this->m_is_ssl = other.m_is_ssl;
            this->m_port = other.m_port;
            this->m_recv_len = other.m_recv_len;
        }

        // void show() override ;
        // void show(const std::string& filepath) override;

        uint16_t m_port;
        uint32_t m_backlog;
        IO_ENGINE m_engine;
        uint32_t m_event_size = DEFAULT_EVENT_SIZE;
        uint32_t m_event_time_out = DEFAULT_EVENT_TIME_OUT;
        uint32_t m_recv_len = DEFAULT_RECV_LENGTH;
        bool m_is_ssl;
    };

    class Tcp_Client_Config: public Config
    {
    public:
        

    };

    //http server 配置类
    class Http_Server_Config: public Tcp_Server_Config 
    {
    public:
        using ptr = std::shared_ptr<Http_Server_Config>;
        Http_Server_Config(uint16_t port,uint32_t backlog,IO_ENGINE engine,bool is_ssl = false):
            Tcp_Server_Config(port , backlog , engine , is_ssl)
        {

        }

    };



};



#endif