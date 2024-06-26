#ifndef GALAY_BUILDER_H
#define GALAY_BUILDER_H
#include <atomic>
#include <map>
#include <functional>
#include <openssl/ssl.h>
#include <spdlog/spdlog.h>
#include "controller.h"

namespace galay
{
    namespace kernel
    {
        class GY_SSLConfig
        {
        public:
            using ptr = std::shared_ptr<GY_SSLConfig>;
            using wptr = std::weak_ptr<GY_SSLConfig>;
            using uptr = std::unique_ptr<GY_SSLConfig>;
            inline GY_SSLConfig();
            // inline int Init();
            inline void SetSSLVersion(int32_t min_ssl_version, int32_t max_ssl_version);
            inline void SetCertPath(const std::string &cert_path);
            inline void SetKeyPath(const std::string &key_path);
            inline int32_t GetMinSSLVersion() const;
            inline int32_t GetMaxSSLVersion() const;
            inline std::string GetCertPath() const;
            inline std::string GetKeyPath() const;
            // inline SSL_CTX* GetSSLContext() const;
        private:
            std::atomic_int32_t m_min_ssl_version;
            std::atomic_int32_t m_max_ssl_version;
            std::string m_ssl_cert_path;
            std::string m_ssl_key_path;
            // std::atomic<SSL_CTX*> m_ssl_ctx;
        };

        class GY_TcpServerBuilderBase
        {
        public:
            using ptr = std::shared_ptr<GY_TcpServerBuilderBase>;
            using uptr = std::unique_ptr<GY_TcpServerBuilderBase>;
            using wptr = std::weak_ptr<GY_TcpServerBuilderBase>;
            virtual void SetUserFunction(std::pair<uint16_t, std::function<common::GY_TcpCoroutine<common::CoroutineStatus>(GY_Controller::wptr)>> port_func) = 0;

            //option
            virtual void SetSchedulerType(galay::common::SchedulerType scheduler_type) = 0;
            virtual void SetThreadNum(uint16_t threadnum) = 0;
            virtual void SetBacklog(uint16_t backlog) = 0;
            virtual void SetMaxEventSize(uint16_t max_event_size) = 0;
            virtual void SetScheWaitTime(uint16_t sche_wait_time) = 0;
            virtual void SetReadBufferLen(uint32_t rbuffer_len) = 0;
            virtual void SetPort(uint16_t port) = 0;

            //SSL env
            virtual void InitSSLServer(bool is_ssl) = 0;
            virtual const GY_SSLConfig::ptr GetSSLConfig() = 0;

            // 每一个端口对应的线程数
            virtual uint16_t GetThreadNum() = 0;
            virtual uint16_t GetBacklog() = 0;
            virtual galay::common::SchedulerType GetSchedulerType() = 0;
            virtual uint16_t GetMaxEventSize() = 0;
            virtual uint16_t GetScheWaitTime() = 0;
            virtual uint32_t GetReadBufferLen() = 0;
            virtual uint16_t GetPort() = 0;
            virtual std::function<common::GY_TcpCoroutine<common::CoroutineStatus>(GY_Controller::wptr)> GetUserFunction() = 0;
            virtual bool GetIsSSL() = 0;
            virtual std::string GetTypeName(galay::common::ClassNameType type) = 0;
            ~GY_TcpServerBuilderBase() = default;
        };

        template <galay::common::TcpRequest REQ, galay::common::TcpResponse RESP>
        class GY_TcpServerBuilder : public GY_TcpServerBuilderBase
        {
        public:
            using ptr = std::shared_ptr<GY_TcpServerBuilder>;
            using wptr = std::weak_ptr<GY_TcpServerBuilder>;
            using uptr = std::unique_ptr<GY_TcpServerBuilder>;
            GY_TcpServerBuilder();
            virtual void SetUserFunction(std::pair<uint16_t, std::function<common::GY_TcpCoroutine<common::CoroutineStatus>(GY_Controller::wptr)>> port_func) override;
            //option
            virtual void SetSchedulerType(galay::common::SchedulerType scheduler_type) override;
            virtual void SetThreadNum(uint16_t threadnum) override;
            virtual void SetBacklog(uint16_t backlog) override;
            virtual void SetMaxEventSize(uint16_t max_event_size) override;
            virtual void SetScheWaitTime(uint16_t sche_wait_time) override;
            virtual void SetReadBufferLen(uint32_t rbuffer_len) override;
            virtual void SetPort(uint16_t port) override;
            //开启SSL env
            virtual void InitSSLServer(bool is_ssl) override;
            virtual const GY_SSLConfig::ptr GetSSLConfig() override;
            // 每一个端口对应的线程数
            virtual uint16_t GetThreadNum() override;
            virtual uint16_t GetBacklog() override;
            virtual galay::common::SchedulerType GetSchedulerType() override;
            virtual uint16_t GetMaxEventSize() override;
            virtual uint16_t GetScheWaitTime() override;
            virtual uint32_t GetReadBufferLen() override;
            virtual uint16_t GetPort() override;
            virtual std::function<common::GY_TcpCoroutine<common::CoroutineStatus>(GY_Controller::wptr)> GetUserFunction() override;
            virtual bool GetIsSSL() override;
            virtual std::string GetTypeName(galay::common::ClassNameType type) override;
            ~GY_TcpServerBuilder();

        protected:
            std::atomic_char16_t m_backlog;
            std::atomic_uint16_t m_port;
            std::atomic_uint16_t m_threadnum;
            std::atomic_uint32_t m_rbuffer_len;
            std::atomic_uint16_t m_max_event_size;
            std::atomic_int16_t m_sche_wait_time;
            std::atomic<galay::common::SchedulerType> m_scheduler_type;
            std::atomic_bool m_is_ssl;
            std::function<common::GY_TcpCoroutine<common::CoroutineStatus>(GY_Controller::wptr)> m_userfunc;
            GY_SSLConfig::ptr m_ssl_config;

            std::unordered_map<int, std::string> m_typeNames;
        };

        class GY_HttpRouter;
 
        class GY_HttpServerBuilder : public GY_TcpServerBuilder<galay::protocol::http::Http1_1_Request, galay::protocol::http::Http1_1_Response>
        {
        public:
            using ptr = std::shared_ptr<GY_HttpServerBuilder>;
            using wptr = std::weak_ptr<GY_HttpServerBuilder>;
            using uptr = std::unique_ptr<GY_HttpServerBuilder>;
            GY_HttpServerBuilder() = default;
            virtual std::function<common::GY_TcpCoroutine<common::CoroutineStatus>(GY_Controller::wptr)> GetUserFunction() override;
            void SetRouter(std::shared_ptr<GY_HttpRouter> router);

        private:
            virtual void SetUserFunction(std::pair<uint16_t, std::function<common::GY_TcpCoroutine<common::CoroutineStatus>(GY_Controller::wptr)>> port_func) override;
        private:
            std::shared_ptr<GY_HttpRouter> m_router;
        };

#include "builder.inl"
    }

}

#endif