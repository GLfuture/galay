#ifndef GALAY_CONTROLLER_H
#define GALAY_CONTROLLER_H
#include <functional>
#include <any>
#include "../common/base.h"
#include "../common/result.h"
#include "../common/waitgroup.h"

namespace galay::poller
{
    class IOScheduler;
    class TcpConnector;
    class Timer;
}

namespace galay::result 
{   
    class ResultInterface;    
}

namespace galay::server
{
    class NetResult
    {
    public:
        using ptr = std::shared_ptr<NetResult>;
        NetResult(result::ResultInterface::ptr result);
        bool Success();
        std::string Error();
        coroutine::GroupAwaiter& Wait();
    private:
        result::ResultInterface::ptr m_result;
    };

    class Controller: public std::enable_shared_from_this<Controller>
    {
    public:
        using ptr = std::shared_ptr<Controller>;
        using wptr = std::weak_ptr<Controller>;
        using uptr = std::unique_ptr<Controller>;
        Controller(std::weak_ptr<poller::TcpConnector> connector);
        //关闭连接
        void Close();
        void PopRequest();
        //获取请求队列的第一个请求
        protocol::Request::ptr GetRequest();
        //获取之前存入的上下文
        std::any &GetContext();
        void SetContext(std::any &&context);
        //发送数据 返会一个NetResult 需要调用NetResult.Wait()
        NetResult Send(std::string &&response);
        std::shared_ptr<poller::Timer> AddTimer(uint64_t during, uint32_t exec_times, std::function<void(std::shared_ptr<poller::Timer>)> &&func);
    private:
        std::any m_context;
        std::weak_ptr<poller::TcpConnector> m_connector;
    };

    class HttpController
    {
    public:
        using ptr = std::shared_ptr<HttpController>;
        using wptr = std::weak_ptr<HttpController>;
        using uptr = std::unique_ptr<HttpController>;
        HttpController(Controller::ptr ctrl);
        void Close();
        void SetContext(std::any &&context);
        std::any &GetContext();
        //获取请求并出队
        protocol::http::HttpRequest::ptr GetRequest();
        NetResult Send(std::string &&response);
        std::shared_ptr<poller::Timer> AddTimer(uint64_t during, uint32_t exec_times, std::function<void(std::shared_ptr<poller::Timer>)> &&func);
    private:
        Controller::ptr m_ctrl;
    };
}

#endif