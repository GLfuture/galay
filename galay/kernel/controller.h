#ifndef GALAY_CONTROLLER_H
#define GALAY_CONTROLLER_H
#include "coroutine.h"
#include "awaiter.h"
#include "waitgroup.h"
#include <functional>
#include <any>

namespace galay {
    class Timer;
    class GY_IOScheduler;
    class GY_Connector;
    
    class GY_Controller: public std::enable_shared_from_this<GY_Controller>
    {
        friend class GY_Connector;
    public:
        using ptr = ::std::shared_ptr<GY_Controller>;
        using wptr = ::std::weak_ptr<GY_Controller>;
        using uptr = ::std::unique_ptr<GY_Controller>;
        GY_Controller(::std::weak_ptr<GY_Connector> connector);
        void Close();
        bool IsClosed();
        void SetContext(::std::any&& context);
        protocol::GY_TcpRequest::ptr GetRequest();
        void PushResponse(protocol::GY_TcpResponse::ptr response);
        ::std::any&& GetContext();
        ::std::shared_ptr<Timer> AddTimer(uint64_t during, uint32_t exec_times,::std::function<::std::any()> &&func);
        //协程结束时必须调用
        void Done();
    protected:
        void SetWaitGroup(WaitGroup* waitgroup);
    private:
        ::std::weak_ptr<GY_Connector> m_connector;
        bool m_close;
        WaitGroup* m_group;
    };

    class GY_HttpController{
        friend class GY_HttpServerBuilder;
    public:
        using ptr = ::std::shared_ptr<GY_HttpController>;
        using wptr = ::std::weak_ptr<GY_HttpController>;
        using uptr = ::std::unique_ptr<GY_HttpController>;
        GY_HttpController(GY_Controller::wptr ctrl);
        void SetContext(::std::any&& context);
        ::std::any&& GetContext();
        ::std::shared_ptr<Timer> AddTimer(uint64_t during, uint32_t exec_times,::std::function<::std::any()> &&func);
        void Done();
        void Close();
        protocol::http::Http1_1_Request::ptr GetRequest();
        void PushResponse(protocol::http::Http1_1_Response::ptr response);
    protected:
        void SetWaitGroup(WaitGroup* waitgroup);
        void SetRequest(protocol::http::Http1_1_Request::ptr request);
    private:
        GY_Controller::wptr m_ctrl;
        WaitGroup* m_waitgroup;
        protocol::http::Http1_1_Request::ptr m_request;
    };
}

#endif