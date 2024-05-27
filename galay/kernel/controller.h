#ifndef GALAY_CONTROLLER_H
#define GALAY_CONTROLLER_H
#include "coroutine.h"
#include "awaiter.h"
#include <functional>
#include <any>

namespace galay {
    class Timer;
    class GY_IOScheduler;
    class GY_UnaryStrategy;
    class GY_Connector;
    
    class GY_Controller: public std::enable_shared_from_this<GY_Controller>
    {
        friend class GY_UnaryStrategy;
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

    private:
        ::std::weak_ptr<GY_Connector> m_connector;
        bool m_close;
    };
}

#endif