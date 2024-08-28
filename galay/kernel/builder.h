#ifndef GALAY_BUILDER_H
#define GALAY_BUILDER_H
#include <atomic>
#include <map>
#include <functional>
#include <openssl/ssl.h>
#include <spdlog/spdlog.h>
#include "controller.h"
#include "../util/io.h"
namespace galay::coroutine
{
    class NetCoroutine;
}

namespace galay::server
{
    enum ClassNameType {
        kClassNameRequest,
        kClassNameResponse
    };

    enum PollerType {
        kSelectPoller,
        kEpollPoller
    };

    class TcpServerBuilderBase
    {
    public:
        using ptr = std::shared_ptr<TcpServerBuilderBase>;
        using uptr = std::unique_ptr<TcpServerBuilderBase>;
        using wptr = std::weak_ptr<TcpServerBuilderBase>;
        virtual void SetPort(uint16_t port) = 0;
        virtual void SetRightHandle(std::function<coroutine::NetCoroutine(Controller::ptr)> port_func) = 0;
        virtual void SetWrongHandle(std::function<coroutine::NetCoroutine(Controller::ptr)> handle) = 0;
        //option
        virtual void SetPollerType(PollerType pollerType) = 0;
        virtual void SetNetThreadNum(uint16_t threadnum) = 0;
        virtual void SetBacklog(uint16_t backlog) = 0;
        virtual void SetMaxEventSize(uint16_t max_event_size) = 0;
        virtual void SetScheWaitTime(uint16_t sche_wait_time) = 0;

        //SSL env
        virtual void InitSSLServer(io::net::SSLConfig::ptr config) = 0;
        virtual const io::net::SSLConfig::ptr GetSSLConfig() = 0;

        // 每一个端口对应的线程数
        virtual uint16_t GetNetThreadNum() = 0;
        virtual uint16_t GetBacklog() = 0;
        virtual PollerType GetSchedulerType() = 0;
        virtual uint16_t GetMaxEventSize() = 0;
        virtual uint16_t GetScheWaitTime() = 0;
        virtual uint16_t GetPort() = 0;
        virtual bool GetIsSSL() = 0;
        virtual std::string GetTypeName(ClassNameType type) = 0;
        ~TcpServerBuilderBase() = default;
    };

    class TcpServerBuilder: public TcpServerBuilderBase
    {
    public:
        using ptr = std::shared_ptr<TcpServerBuilder>;
        using wptr = std::weak_ptr<TcpServerBuilder>;
        using uptr = std::unique_ptr<TcpServerBuilder>;
        TcpServerBuilder();
        template <common::TcpRequest REQ, common::TcpResponse RESP>
        bool InitProtocol()
        {
            m_typeNames[kClassNameRequest] = util::GetTypeName<REQ>() ;
            m_typeNames[kClassNameResponse] = util::GetTypeName<RESP>() ;
            if(m_typeNames[kClassNameRequest].empty() || m_typeNames[kClassNameResponse].empty()) {
                spdlog::error("[{}:{}] [protocol type name is empty]", __FILE__, __LINE__);
                return false;
            }
            return true;
        }
        virtual void SetRightHandle(std::function<coroutine::NetCoroutine(Controller::ptr)> handle) override;
        virtual void SetWrongHandle(std::function<coroutine::NetCoroutine(Controller::ptr)> handle) override;
        //option
        virtual void SetPollerType(PollerType pollerType) override;
        virtual void SetNetThreadNum(uint16_t threadnum) override;
        virtual void SetBacklog(uint16_t backlog) override;
        virtual void SetMaxEventSize(uint16_t max_event_size) override;
        virtual void SetScheWaitTime(uint16_t sche_wait_time) override;
        virtual void SetPort(uint16_t port) override;
        //开启SSL env
        virtual void InitSSLServer(io::net::SSLConfig::ptr config) override;
        virtual const io::net::SSLConfig::ptr GetSSLConfig() override;
        // 每一个端口对应的线程数
        virtual uint16_t GetNetThreadNum() override;
        virtual uint16_t GetBacklog() override;
        virtual PollerType GetSchedulerType() override;
        virtual uint16_t GetMaxEventSize() override;
        virtual uint16_t GetScheWaitTime() override;
        virtual uint16_t GetPort() override;
        virtual bool GetIsSSL() override;
        virtual std::string GetTypeName(ClassNameType type) override;
        ~TcpServerBuilder();

    protected:
        std::atomic_char16_t m_backlog;
        std::atomic_uint16_t m_port;
        std::atomic_uint16_t m_threadnum;
        std::atomic_uint32_t m_rbuffer_len;
        std::atomic_uint16_t m_max_event_size;
        std::atomic_int16_t m_sche_wait_time;
        std::atomic<PollerType> m_pollerType;
        io::net::SSLConfig::ptr m_ssl_config;

        std::unordered_map<int, std::string> m_typeNames;
    };

    class HttpRouter;

    class HttpServerBuilder: public TcpServerBuilder
    {
    public:
        using ptr = std::shared_ptr<HttpServerBuilder>;
        using wptr = std::weak_ptr<HttpServerBuilder>;
        using uptr = std::unique_ptr<HttpServerBuilder>;
        HttpServerBuilder();
        void SetRouter(std::shared_ptr<HttpRouter> router);

    private:
        virtual void SetRightHandle(std::function<coroutine::NetCoroutine(Controller::ptr)> handle) override;
        virtual void SetWrongHandle(std::function<coroutine::NetCoroutine(Controller::ptr)> handle) override;
    private:
        std::shared_ptr<HttpRouter> m_router;
    };

}



#endif