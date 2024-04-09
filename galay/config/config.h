#ifndef GALAY_CONFIG_H
#define GALAY_CONFIG_H

#include <iostream>
#include <memory>
#include <string>
#include <cxxabi.h>
#include <fstream>
#include <functional>
#include <vector>
#include <openssl/ssl.h>

namespace galay{
    #define DEFAULT_EVENT_SIZE                      2048
    #define DEFAULT_EVENT_TIME_OUT                  5
    #define DEFAULT_RECV_LENGTH                     1024
    #define DEFAULT_BACKLOG                         128        
    #define DEFAULT_MAX_SSL_ACCEPT_RETRY            1000
    #define DEFAULT_SSL_SLEEP_MISC_PER_RETRY        1
    #define DEFAULT_CLINET_EVENT_SIZE               1
    #define MAX_UDP_LENGTH                          1400
    #define MAX_UDP_WAIT_FOR_RECV_TIME              5000
    #define DEFAULT_CONNCTION_TIMEOUT               -1

    #define DEFAULT_SSL_MIN_VERSION                 TLS1_2_VERSION
    #define DEFAULT_SSL_MAX_VERSION                 TLS1_2_VERSION

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

    struct TcpKeepaliveConf
    {
        bool m_keepalive = false;
        int m_idle;                 //无包时发送保活包开始时间   单位s
        int m_interval;             //保活包间隔                单位s
        int m_retry;                //重试次数
    };


    struct TcpConnTimeOut{
        int m_timeout = -1;         //单位：ms
    };

    struct TcpSSLConf
    {
        long m_min_version = DEFAULT_SSL_MIN_VERSION;
        long m_max_version = DEFAULT_SSL_MAX_VERSION;
        uint32_t m_max_accept_retry = 50;
        uint32_t m_sleep_misc_per_retry = 1;
        std::string m_cert_filepath;
        std::string m_key_filepath;
    };

    //tcp server配置类
    class TcpServerConf: public Config
    {
    public:
        using ptr = std::shared_ptr<TcpServerConf>;
        //port 端口 backlog 全连接队列大小 recv_len 默认接收缓冲区大小  conn_timeout 超时断连时间(-1 为不开启)
        TcpServerConf(uint32_t backlog,uint32_t recv_len,Engine_Type type,int sche_wait_time,int max_event_size,int threadnum);

        TcpServerConf(TcpServerConf&& other);

        TcpServerConf(const TcpServerConf& other);

        //设置keepalive
        //t_idle  单位：s
        //t_interval 单位：s
        //retry 重试次数
        void set_keepalive(int t_idle,int t_interval,int retry);
        //设置连接超时
        //t_timeout 单位：ms
        void set_conn_timeout(int t_timeout);

        uint32_t m_backlog;
        uint32_t m_max_rbuffer_len;
        TcpConnTimeOut m_conn_timeout;
        TcpKeepaliveConf m_keepalive_conf;
    };


    //tcp ssl 配置类
    class TcpSSLServerConf: public TcpServerConf
    {
    public:
        using ptr = std::shared_ptr<TcpSSLServerConf>;
        TcpSSLServerConf(uint32_t backlog ,uint32_t recv_len, Engine_Type type,int sche_wait_time,int max_event_size,int threadnum);

        TcpSSLServerConf(const TcpSSLServerConf &other);

        TcpSSLServerConf(TcpSSLServerConf &&other);

        void set_ssl_conf(long min_version,long max_version,uint32_t max_accept_retry,uint32_t sleep_misc_per_retry,const char* cert_filepath,const char* key_filepath);
        
        TcpSSLConf m_ssl_conf;
    };

    //http server 配置类
    class HttpServerConf: public TcpServerConf 
    {
    public:
        using ptr = std::shared_ptr<HttpServerConf>;
        HttpServerConf(uint32_t backlog ,uint32_t recv_len,Engine_Type type,int sche_wait_time,int max_event_size,int threadnum);
        HttpServerConf(const HttpServerConf &other);
        HttpServerConf(HttpServerConf &&other);
    };

    class HttpSSLServerConf: public TcpSSLServerConf
    {
    public:
        using ptr = std::shared_ptr<HttpSSLServerConf>;
        HttpSSLServerConf(uint32_t backlog ,uint32_t recv_len,Engine_Type type,int sche_wait_time,int max_event_size,int threadnum);
        HttpSSLServerConf(const HttpSSLServerConf &other);
        HttpSSLServerConf(HttpSSLServerConf &&other);
    };

    class UdpServerConf: public Config
    {
    public:
        using ptr = std::shared_ptr<UdpServerConf>;
        UdpServerConf(Engine_Type type,int sche_wait_time,int max_event_size,int threadnum);
        UdpServerConf(UdpServerConf&& other);
        UdpServerConf(const UdpServerConf& other);

    };


};



#endif