#ifndef GALAY_CONFIG_H
#define GALAY_CONFIG_H

#include <iostream>
#include <memory>
#include <string>
#include <cxxabi.h>
#include <fstream>
#include <functional>

namespace galay{
    #define DEFAULT_EVENT_SIZE                      2048
    #define DEFAULT_EVENT_TIME_OUT                  5
    #define DEFAULT_RECV_LENGTH                     1024
    #define DEFAULT_FD_NUM                          2000
    #define DEFAULT_BACKLOG                         256        
    #define DEFAULT_ENGINE                          IO_ENGINE::IO_EPOLL
    #define DEFAULT_MAX_SSL_ACCEPT_RETRY            1000
    #define DEFAULT_SSL_SLEEP_MISC_PER_RETRY        1


    
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
        Tcp_Server_Config(uint16_t port,uint32_t backlog,IO_ENGINE engine) : 
            m_port(port),m_backlog(backlog),m_engine(engine)
        {
            
        } 

        Tcp_Server_Config(Tcp_Server_Config&& other)
        {
            this->m_backlog = other.m_backlog;
            this->m_engine = other.m_engine;
            this->m_event_size = other.m_event_size;
            this->m_event_time_out = other.m_event_time_out;
            this->m_port = other.m_port;
            this->m_recv_len = other.m_recv_len;
        }

        Tcp_Server_Config(const Tcp_Server_Config& other)
        {
            this->m_backlog = other.m_backlog;
            this->m_engine = other.m_engine;
            this->m_event_size = other.m_event_size;
            this->m_event_time_out = other.m_event_time_out;
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
    };

    class Tcp_SSL_Server_Config: public Tcp_Server_Config
    {
    public:
        using ptr = std::shared_ptr<Tcp_SSL_Server_Config>;
        Tcp_SSL_Server_Config(uint16_t port,uint32_t backlog,IO_ENGINE engine , long ssl_min_version , long ssl_max_version
            , uint32_t ssl_accept_max_retry ,const char* cert_filepath,const char* key_filepath):
            Tcp_Server_Config(port,backlog,engine),m_ssl_min_version(ssl_min_version),m_ssl_max_version(ssl_max_version),
               m_ssl_accept_retry(ssl_accept_max_retry) ,m_cert_filepath(cert_filepath),m_key_filepath(key_filepath)
        {

        }

        Tcp_SSL_Server_Config(const Tcp_SSL_Server_Config &other)
            :Tcp_Server_Config(other)
        {
            this->m_ssl_min_version = other.m_ssl_min_version;
            this->m_ssl_max_version = other.m_ssl_max_version;
            this->m_cert_filepath = other.m_cert_filepath;
            this->m_key_filepath = other.m_key_filepath;
            this->m_ssl_accept_retry = other.m_ssl_accept_retry;
        }

        Tcp_SSL_Server_Config(Tcp_SSL_Server_Config &&other)
            :Tcp_Server_Config(std::forward<Tcp_SSL_Server_Config>(other))
        {
            this->m_ssl_min_version = other.m_ssl_min_version;
            this->m_ssl_max_version = other.m_ssl_max_version;
            this->m_cert_filepath = std::move(other.m_cert_filepath);
            this->m_key_filepath = std::move(other.m_key_filepath);
            this->m_ssl_accept_retry = other.m_ssl_accept_retry;
        }

        std::string m_cert_filepath;
        std::string m_key_filepath;
        long m_ssl_min_version;
        long m_ssl_max_version;
        uint32_t m_ssl_accept_retry;
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
        Http_Server_Config(uint16_t port,uint32_t backlog,IO_ENGINE engine):
            Tcp_Server_Config(port , backlog , engine)
        {

        }

    };



};



#endif