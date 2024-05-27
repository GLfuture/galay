#ifndef GALAY_SERVER_H
#define GALAY_SERVER_H

#include "task.h"
#include <thread>


namespace galay {

    class GY_Server {
    public:
        using ptr = ::std::shared_ptr<GY_Server>;
        using wptr = ::std::weak_ptr<GY_Server>;
        using uptr = ::std::unique_ptr<GY_Server>;
        virtual void Start(GY_TcpServerBuilderBase::ptr builder) = 0;
        virtual void Stop() = 0;
        virtual GY_IOScheduler::wptr GetScheduler(int indx) = 0;
        virtual ~GY_Server() = default;
    };

    class GY_TcpServer: public GY_Server {
    public:
        using ptr = ::std::shared_ptr<GY_TcpServer>;
        using wptr = ::std::weak_ptr<GY_TcpServer>;
        using uptr = ::std::unique_ptr<GY_TcpServer>;
        GY_TcpServer() = default;
        virtual void Start(GY_TcpServerBuilderBase::ptr builder) override;
        virtual void Stop() override;
        virtual GY_IOScheduler::wptr GetScheduler(int indx) override;
        virtual ~GY_TcpServer();
    private:
        galay::GY_IOScheduler::ptr CreateScheduler(GY_TcpServerBuilderBase::ptr builder);
        galay::GY_TimerManager::ptr CreateTimerManager();
        galay::GY_Acceptor::ptr CreateAcceptor(GY_IOScheduler::ptr scheduler);
        void WaitForThreads();
    private:
        ::std::vector<::std::shared_ptr<::std::thread>> m_threads; 
        ::std::vector<GY_IOScheduler::ptr> m_schedulers;
    };

    using GY_HttpServer = GY_TcpServer;
}

#endif