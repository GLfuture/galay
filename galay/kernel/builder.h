#ifndef GALAY_BUILDER_H
#define GALAY_BUILDER_H
#include <atomic>
#include <map>
#include <functional>
#include <openssl/ssl.h>
#include "controller.h"
#include "global.h"

namespace galay {

    class GY_SSLConfig {
    public:
        using ptr = std::shared_ptr<GY_SSLConfig>;
        using wptr = std::weak_ptr<GY_SSLConfig>;
        using uptr = std::unique_ptr<GY_SSLConfig>;
        inline GY_SSLConfig();
        //inline int Init();
        inline void SetSSLVersion(int32_t min_ssl_version, int32_t max_ssl_version);
        inline void SetCertPath(const std::string& cert_path);
        inline void SetKeyPath(const std::string& key_path);
        inline int32_t GetMinSSLVersion() const;
        inline int32_t GetMaxSSLVersion() const;
        inline std::string GetCertPath() const;
        inline std::string GetKeyPath() const;
        //inline SSL_CTX* GetSSLContext() const;
    private:
        std::atomic_int32_t m_min_ssl_version;
        std::atomic_int32_t m_max_ssl_version;
        std::string m_ssl_cert_path;
        std::string m_ssl_key_path;
        //std::atomic<SSL_CTX*> m_ssl_ctx;
    };
    
    class GY_TcpServerBuilderBase {
    public:
        using ptr = std::shared_ptr<GY_TcpServerBuilderBase>;
        using uptr = std::unique_ptr<GY_TcpServerBuilderBase>;
        using wptr = std::weak_ptr<GY_TcpServerBuilderBase>;
        enum SchedulerType
        {
            SELECT_SCHEDULER,       //select
            EPOLL_SCHEDULER,        //epoll
        };
        //nessaray
        virtual void SetIllegalFunction(::std::function<GY_TcpCoroutine<galay::CoroutineStatus>(std::string&,std::string&)> func) = 0;
        virtual void SetUserFunction(::std::pair<uint16_t,::std::function<GY_TcpCoroutine<galay::CoroutineStatus>(GY_Controller::wptr)>> port_func) = 0;
        virtual void InitSSLServer(bool is_ssl) = 0;
        virtual void SetSchedulerType(SchedulerType scheduler_type) = 0;
        virtual void SetThreadNum(uint16_t threadnum) = 0;
        virtual void SetBacklog(uint16_t backlog) = 0;
        virtual void SetMaxEventSize(uint16_t max_event_size) = 0;
        virtual void SetScheWaitTime(uint16_t sche_wait_time) = 0;
        virtual void SetReadBufferLen(uint32_t rbuffer_len) = 0;

        //每一个端口对应的线程数
        virtual uint16_t GetThreadNum() = 0;
        virtual uint16_t GetBacklog() = 0;
        virtual SchedulerType GetSchedulerType() = 0;
        virtual uint16_t GetMaxEventSize() = 0;
        virtual uint16_t GetScheWaitTime() = 0;
        virtual uint32_t GetReadBufferLen() = 0;
        virtual uint16_t GetPort() = 0;
        virtual ::std::function<GY_TcpCoroutine<galay::CoroutineStatus>(GY_Controller::wptr)> GetUserFunction() = 0;
        virtual ::std::function<GY_TcpCoroutine<galay::CoroutineStatus>(std::string&,std::string&)> GetIllegalFunction() = 0;
        virtual bool GetIsSSL() = 0;
        virtual GY_SSLConfig::ptr GetSSLConfig() = 0;
        ~GY_TcpServerBuilderBase() = default;
    };

    template<Tcp_Request REQ, Tcp_Response RESP>
    class GY_TcpServerBuilder : public GY_TcpServerBuilderBase
    {
    public:
        using ptr = ::std::shared_ptr<GY_TcpServerBuilder>;
        using wptr = ::std::weak_ptr<GY_TcpServerBuilder>;
        using uptr = ::std::unique_ptr<GY_TcpServerBuilder>;
        GY_TcpServerBuilder();
        //nessaray
        virtual void  SetUserFunction(::std::pair<uint16_t,::std::function<GY_TcpCoroutine<galay::CoroutineStatus>(GY_Controller::wptr)>> port_func) override;
        virtual void  SetIllegalFunction(::std::function<GY_TcpCoroutine<galay::CoroutineStatus>(std::string&,std::string&)> func) override;
        virtual void  SetSchedulerType(SchedulerType scheduler_type) override;
        virtual void  SetThreadNum(uint16_t threadnum) override;
        virtual void  SetBacklog(uint16_t backlog) override;
        virtual void  SetMaxEventSize(uint16_t max_event_size) override;
        virtual void  SetScheWaitTime(uint16_t sche_wait_time) override;
        virtual void  SetReadBufferLen(uint32_t rbuffer_len) override; 
        virtual void InitSSLServer(bool is_ssl) override;
        //每一个端口对应的线程数
        virtual uint16_t GetThreadNum() override;
        virtual uint16_t GetBacklog() override;
        virtual SchedulerType GetSchedulerType() override;
        virtual uint16_t GetMaxEventSize() override;
        virtual uint16_t GetScheWaitTime() override;
        virtual uint32_t GetReadBufferLen() override;
        virtual uint16_t GetPort() override;
        virtual ::std::function<GY_TcpCoroutine<galay::CoroutineStatus>(GY_Controller::wptr)> GetUserFunction() override;
        virtual ::std::function<GY_TcpCoroutine<galay::CoroutineStatus>(std::string&,std::string&)> GetIllegalFunction() override;
        virtual bool GetIsSSL() override;
        virtual GY_SSLConfig::ptr GetSSLConfig() override;
        ~GY_TcpServerBuilder();
    private:
        ::std::atomic_char16_t m_backlog;
        ::std::atomic_uint16_t m_port;    
        ::std::atomic_uint16_t m_threadnum;
        ::std::atomic_uint32_t m_rbuffer_len;
        ::std::atomic_uint16_t m_max_event_size;
        ::std::atomic_int16_t m_sche_wait_time;
        ::std::atomic<SchedulerType> m_scheduler_type;
        ::std::atomic_bool m_is_ssl;
        ::std::function<GY_TcpCoroutine<galay::CoroutineStatus>(GY_Controller::wptr)> m_userfunc;
        ::std::function<GY_TcpCoroutine<galay::CoroutineStatus>(std::string&,std::string&)> m_illegalfunc;
        ::std::atomic<GY_SSLConfig::ptr> m_ssl_config;
    };

    class GY_HttpServerBuilder: public GY_TcpServerBuilder<protocol::http::Http1_1_Request,protocol::http::Http1_1_Response>
    {
    public:
        
    };
    
    #include "builder.inl"
}


#endif