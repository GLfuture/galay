#ifndef GALAY_AWAIT_H
#define GALAY_AWAIT_H

#include <memory>
#include <coroutine>
#include <any>
#include <optional>
#include <functional>
#include <future>
#include "../protocol/http1_1.h"
#include "../protocol/smtp.h"
#include "../protocol/dns.h"

namespace galay
{
    namespace common
    {
        // return int
        class AwaiterBase
        {
        public:
            virtual bool await_ready() = 0;
            virtual void await_suspend(std::coroutine_handle<> handle) = 0;
            virtual std::any await_resume() = 0;
        };

        class CommonAwaiter : public AwaiterBase
        {
        public:
            CommonAwaiter(bool IsSuspend, const std::any &result);
            CommonAwaiter(CommonAwaiter &&other);
            CommonAwaiter &operator=(const CommonAwaiter &) = delete;
            CommonAwaiter &operator=(CommonAwaiter &&other);
            virtual bool await_ready() override;
            virtual void await_suspend(std::coroutine_handle<> handle) override;
            virtual std::any await_resume() override;

        private:
            std::any m_Result;
            bool m_IsSuspend;
        };

        class GroupAwaiter
        {
        public:
            GroupAwaiter() = default;
            GroupAwaiter(std::weak_ptr<common::GY_NetCoroutinePool> coPool, bool IsSuspend);
            GroupAwaiter(GroupAwaiter &&other);
            GroupAwaiter &operator=(const GroupAwaiter &other);
            GroupAwaiter &operator=(GroupAwaiter &&other);
            void Resume();
            bool await_ready();
            void await_suspend(std::coroutine_handle<> handle);
            std::any await_resume();

        private:
            bool m_IsSuspend;
            std::weak_ptr<common::GY_NetCoroutinePool> m_coPool;
            uint64_t m_coId;
        };

        class HttpAwaiter
        {
        public:
            HttpAwaiter(bool IsSuspend, std::function<protocol::http::Http1_1_Response::ptr()> &Func, std::queue<std::future<void>> &futures);
            HttpAwaiter(HttpAwaiter &&other);
            HttpAwaiter &operator=(const HttpAwaiter &) = delete;
            HttpAwaiter &operator=(HttpAwaiter &&other);
            virtual bool await_ready();
            virtual void await_suspend(std::coroutine_handle<> handle);
            virtual protocol::http::Http1_1_Response::ptr await_resume();

        private:
            protocol::http::Http1_1_Response::ptr m_Result;
            bool m_IsSuspend;
            std::queue<std::future<void>> &m_futures;
            std::function<protocol::http::Http1_1_Response::ptr()> &m_Func;
            std::mutex m_mtx;
        };

        class SmtpAwaiter
        {
        public:
            SmtpAwaiter(bool IsSuspend, std::function<std::vector<protocol::smtp::Smtp_Response::ptr>()> &func, std::queue<std::future<void>> &futures);
            SmtpAwaiter(SmtpAwaiter &&other);
            SmtpAwaiter &operator=(const SmtpAwaiter &) = delete;
            SmtpAwaiter &operator=(SmtpAwaiter &&other);
            virtual bool await_ready();
            virtual void await_suspend(std::coroutine_handle<> handle);
            virtual std::vector<protocol::smtp::Smtp_Response::ptr> await_resume();
            ~SmtpAwaiter() = default;

        private:
            bool m_IsSuspend;
            std::queue<std::future<void>> &m_futures;
            std::function<std::vector<protocol::smtp::Smtp_Response::ptr>()> &m_Func;
            std::vector<protocol::smtp::Smtp_Response::ptr> m_Result;
        };

        class DnsAwaiter
        {
        public:
            DnsAwaiter(bool IsSuspend, std::function<galay::protocol::dns::Dns_Response::ptr()> &func, std::queue<std::future<void>> &futures);
            DnsAwaiter(DnsAwaiter &&other);
            DnsAwaiter &operator=(const DnsAwaiter &other) = delete;
            DnsAwaiter &operator=(DnsAwaiter &&other);
            virtual bool await_ready();
            virtual void await_suspend(std::coroutine_handle<> handle);
            virtual protocol::dns::Dns_Response::ptr await_resume();
            ~DnsAwaiter() = default;

        private:
            bool m_IsSuspend;
            std::queue<std::future<void>> &m_futures;
            protocol::dns::Dns_Response::ptr m_Result;
            std::function<galay::protocol::dns::Dns_Response::ptr()> &m_Func;
        };
    }
}

#endif
