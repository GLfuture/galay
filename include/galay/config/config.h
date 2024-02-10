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
    #define DEFAULT_MAX_SSL_ACCEPT_RETRY            1000
    #define DEFAULT_SSL_SLEEP_MISC_PER_RETRY        1
    #define DEFAULT_CLINET_EVENT_SIZE               1
    #define MAX_UDP_LENGTH                          1400
    #define MAX_UDP_WAIT_FOR_RECV_TIME              5000
    #define DEFAULT_CONNCTION_TIMEOUT               -1

    enum Engine_Type{
        ENGINE_EPOLL,
        ENGINE_SELECT,
    };

    class Config
    {
    public:
        using ptr = std::shared_ptr<Config>;
        Config(Engine_Type type,int sche_wait_time,int m_max_event_size,int threadnum);

        Config(Config&& other);

        Config(const Config& other);

        virtual ~Config() {}

        int m_threadnum;
        Engine_Type m_type;
        int m_sche_wait_time;
        int m_max_event_size;
    };

    struct Tcp_Keepalive_Config
    {
        bool m_keepalive = false;
        int m_idle;                 //无包时发送保活包开始时间
        int m_interval;             //保活包间隔
        int m_retry;                //重试次数
    };

    //tcp server配置类
    class Tcp_Server_Config: public Config
    {
    public:
        using ptr = std::shared_ptr<Tcp_Server_Config>;
        //port 端口 backlog 全连接队列大小 recv_len 默认接收缓冲区大小  conn_timeout 超时断连时间(-1 为不开启)
        Tcp_Server_Config(uint16_t port,uint32_t backlog,uint32_t recv_len,int conn_timeout,Engine_Type type,int sche_wait_time,int max_event_size,int threadnum);

        Tcp_Server_Config(Tcp_Server_Config&& other);

        Tcp_Server_Config(const Tcp_Server_Config& other);

        void enable_keepalive(int t_idle,int t_interval,int retry);

        uint16_t m_port;
        uint32_t m_backlog;
        uint32_t m_max_rbuffer_len;
        int m_conn_timeout;
        Tcp_Keepalive_Config  m_keepalive_conf;
    };


    //tcp ssl 配置类
    class Tcp_SSL_Server_Config: public Tcp_Server_Config
    {
    public:
        using ptr = std::shared_ptr<Tcp_SSL_Server_Config>;
        Tcp_SSL_Server_Config(uint16_t port,uint32_t backlog ,uint32_t recv_len , int conn_timeout , long ssl_min_version , long ssl_max_version
            , uint32_t ssl_accept_max_retry ,const char* cert_filepath,const char* key_filepath,Engine_Type type,int sche_wait_time,int max_event_size,int threadnum);

        Tcp_SSL_Server_Config(const Tcp_SSL_Server_Config &other);

        Tcp_SSL_Server_Config(Tcp_SSL_Server_Config &&other);

        std::string m_cert_filepath;
        std::string m_key_filepath;
        long m_ssl_min_version;
        long m_ssl_max_version;
        uint32_t m_ssl_accept_retry;
    };

    //http server 配置类
    class Http_Server_Config: public Tcp_Server_Config 
    {
    public:
        using ptr = std::shared_ptr<Http_Server_Config>;
        Http_Server_Config(uint16_t port,uint32_t backlog ,uint32_t recv_len,int conn_timeout,Engine_Type type,int sche_wait_time,int max_event_size,int threadnum);
        Http_Server_Config(const Http_Server_Config &other);
        Http_Server_Config(Http_Server_Config &&other);
    };

    class Https_Server_Config: public Tcp_SSL_Server_Config
    {
    public:
        using ptr = std::shared_ptr<Https_Server_Config>;
        Https_Server_Config(uint16_t port,uint32_t backlog ,uint32_t recv_len ,int conn_timeout , long ssl_min_version , long ssl_max_version
            , uint32_t ssl_accept_max_retry ,const char* cert_filepath,const char* key_filepath,Engine_Type type,int sche_wait_time,int max_event_size,int threadnum);
        Https_Server_Config(const Https_Server_Config &other);
        Https_Server_Config(Https_Server_Config &&other);
    };

    class Udp_Server_Config: public Config
    {
    public:
        using ptr = std::shared_ptr<Udp_Server_Config>;
        Udp_Server_Config(uint16_t port,Engine_Type type,int sche_wait_time,int max_event_size,int threadnum);
        Udp_Server_Config(Udp_Server_Config&& other);
        Udp_Server_Config(const Udp_Server_Config& other);

        uint16_t m_port;
    };


};



#endif