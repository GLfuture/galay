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
        class TcpResult;

        class GY_Controller: public std::enable_shared_from_this<GY_Controller>
        {
            friend class GY_Connector;
        public:
            using ptr = std::shared_ptr<GY_Controller>;
            using wptr = std::weak_ptr<GY_Controller>;
            using uptr = std::unique_ptr<GY_Controller>;
            GY_Controller(std::weak_ptr<GY_Connector> connector);
            void Close();
            std::any &&GetContext();
            void SetContext(std::any &&context);
            protocol::GY_SRequest::ptr GetRequest();
            void PopRequest();
            std::shared_ptr<TcpResult> Send(std::string &&response);
            std::shared_ptr<Timer> AddTimer(uint64_t during, uint32_t exec_times, std::function<void(std::shared_ptr<Timer>)> &&func);
        private:
            std::weak_ptr<galay::kernel::GY_Connector> m_connector;
        };

        class GY_HttpController
        {
            friend class GY_HttpRouter;
        public:
            using ptr = std::shared_ptr<GY_HttpController>;
            using wptr = std::weak_ptr<GY_HttpController>;
            using uptr = std::unique_ptr<GY_HttpController>;
            GY_HttpController(GY_Controller::ptr ctrl);
            void SetContext(std::any &&context);
            std::any &&GetContext();
            std::shared_ptr<Timer> AddTimer(uint64_t during, uint32_t exec_times, std::function<void(std::shared_ptr<Timer>)> &&func);
            void Close();
            protocol::http::HttpRequest::ptr GetRequest();
            std::shared_ptr<TcpResult> Send(std::string &&response);
        private:
            galay::kernel::GY_Controller::ptr m_ctrl;
        };
    }
}

#endif