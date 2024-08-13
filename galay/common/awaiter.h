#ifndef GALAY_AWAIT_H
#define GALAY_AWAIT_H

#include <memory>
#include <coroutine>
#include <any>
#include <optional>
#include <functional>
#include <future>
#include "../protocol/http.h"
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
            GroupAwaiter(bool IsSuspend);
            GroupAwaiter(GroupAwaiter &&other);
            GroupAwaiter &operator=(const GroupAwaiter &other);
            GroupAwaiter &operator=(GroupAwaiter &&other);
            void Resume();
            bool await_ready();
            void await_suspend(std::coroutine_handle<> handle);
            std::any await_resume();

        private:
            uint64_t m_coId;
            std::atomic_bool m_IsSuspend;
        };
    }
}

#endif
