#ifndef GALAY_CONTROLLER_H
#define GALAY_CONTROLLER_H
#include <functional>
#include <any>
#include "../common/base.h"
#include "../common/waitgroup.h"

namespace galay
{
    namespace kernel
    {
        class Timer;
        class GY_IOScheduler;
        class GY_Connector;

        class GY_Controller : public std::enable_shared_from_this<GY_Controller>
        {
            friend class GY_Connector;

        public:
            using ptr = std::shared_ptr<GY_Controller>;
            using wptr = std::weak_ptr<GY_Controller>;
            using uptr = std::unique_ptr<GY_Controller>;
            GY_Controller(std::weak_ptr<GY_Connector> connector);
            void Close();
            bool IsClosed();
            void SetContext(std::any &&context);
            protocol::GY_Request::ptr GetRequest();
            void PushResponse(std::string &&response);
            std::weak_ptr<common::GY_NetCoroutinePool> GetCoPool();


            std::any &&GetContext();
            std::shared_ptr<Timer> AddTimer(uint64_t during, uint32_t exec_times, std::function<std::any()> &&func);
            // 协程结束时必须调用
            void Done();

        protected:
            void SetWaitGroup(WaitGroup *waitgroup);

        private:
            std::weak_ptr<galay::kernel::GY_Connector> m_connector;
            bool m_close;
            WaitGroup *m_group;
        };

        class GY_HttpController
        {
            friend class GY_HttpRouter;
        public:
            using ptr = std::shared_ptr<GY_HttpController>;
            using wptr = std::weak_ptr<GY_HttpController>;
            using uptr = std::unique_ptr<GY_HttpController>;
            GY_HttpController(GY_Controller::wptr ctrl);
            void SetContext(std::any &&context);
            std::any &&GetContext();
            std::weak_ptr<common::GY_NetCoroutinePool> GetCoPool();
            std::shared_ptr<Timer> AddTimer(uint64_t during, uint32_t exec_times, std::function<std::any()> &&func);
            void Done();
            void Close();
            protocol::http::HttpRequest::ptr GetRequest();
            void PushResponse(std::string &&response);

        protected:
            void SetWaitGroup(WaitGroup *waitgroup);
            void SetRequest(protocol::http::HttpRequest::ptr request);

        private:
            galay::kernel::GY_Controller::wptr m_ctrl;
            galay::kernel::WaitGroup *m_waitgroup;
            protocol::http::HttpRequest::ptr m_request;
        };
    }
}

#endif