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
        class GY_TcpConnector;
        class NetResult;

        class GY_Controller: public std::enable_shared_from_this<GY_Controller>
        {
            friend class GY_TcpConnector;
        public:
            using ptr = std::shared_ptr<GY_Controller>;
            using wptr = std::weak_ptr<GY_Controller>;
            using uptr = std::unique_ptr<GY_Controller>;
            GY_Controller(std::weak_ptr<GY_TcpConnector> connector);
            //关闭连接
            void Close();
            //获取之前存入的上下文
            std::any &GetContext();
            void SetContext(std::any &&context);
            //请求出队
            void PopRequest();
            //获取请求队列的第一个请求
            protocol::GY_Request::ptr GetRequest();
            std::any &GetError();
            //发送数据 返会一个NetResult 需要调用NetResult.Wait()
            std::shared_ptr<NetResult> Send(std::string &&response);
            std::shared_ptr<Timer> AddTimer(uint64_t during, uint32_t exec_times, std::function<void(std::shared_ptr<Timer>)> &&func);
        private:
            void SetError(std::any &&error);
        private:
            std::any m_context;
            std::any m_error;
            std::weak_ptr<galay::kernel::GY_TcpConnector> m_connector;
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
            std::any &GetContext();
            void Close();
            protocol::http::error::HttpError GetError();
            //获取请求并出队
            protocol::http::HttpRequest::ptr GetRequest();
            std::shared_ptr<NetResult> Send(std::string &&response);
            std::shared_ptr<Timer> AddTimer(uint64_t during, uint32_t exec_times, std::function<void(std::shared_ptr<Timer>)> &&func);
        private:
            galay::kernel::GY_Controller::ptr m_ctrl;
        };
    }
}

#endif