#ifndef GALAY_SERVER_H
#define GALAY_SERVER_H

#include "../common/threadpool.h"
#include <thread>

namespace galay::poller
{
    class IOScheduler;
}

namespace galay::objector
{
    class TimerManager;
    class TcpAcceptor;
}

namespace galay::server
{   
    class Controller;
    class TcpServerBuilderBase;
    
    class SIOManager
    {
    public:
        using ptr = std::shared_ptr<SIOManager>;
        using wptr = std::weak_ptr<SIOManager>;
        using uptr = std::unique_ptr<SIOManager>;

        SIOManager(std::shared_ptr<server::TcpServerBuilderBase> builder);
        virtual void Start();
        virtual void Stop();
        virtual std::shared_ptr<poller::IOScheduler> GetIOScheduler();
        virtual ~SIOManager();
    private:
        bool m_stop;
        std::shared_ptr<poller::IOScheduler> m_ioScheduler;
    };

    class Server
    {
    public:
        using ptr = std::shared_ptr<Server>;
        using wptr = std::weak_ptr<Server>;
        using uptr = std::unique_ptr<Server>;
        virtual void Start() = 0;
        virtual void Stop() = 0;
        virtual std::shared_ptr<TcpServerBuilderBase> GetServerBuilder() = 0;
        virtual void SetServerBuilder(std::shared_ptr<TcpServerBuilderBase> builder) = 0;
        virtual std::weak_ptr<SIOManager> GetManager(int indx) = 0;
        virtual ~Server() = default;
    };

    class TcpServer : public Server
    {
    public:
        using ptr = std::shared_ptr<TcpServer>;
        using wptr = std::weak_ptr<TcpServer>;
        using uptr = std::unique_ptr<TcpServer>;
        TcpServer();
        virtual void Start() override;
        virtual void Stop() override;
        virtual std::shared_ptr<TcpServerBuilderBase> GetServerBuilder() override;
        virtual void SetServerBuilder(std::shared_ptr<TcpServerBuilderBase> builder) override;
        virtual std::weak_ptr<SIOManager> GetManager(int indx) override;
        virtual ~TcpServer();
    private:
        std::shared_ptr<TcpServerBuilderBase> m_builder;
        thread::ThreadPool::ptr m_threadPool;
        std::vector<std::shared_ptr<SIOManager>> m_ioManagers;
        std::atomic_bool m_isStopped;
    };
}

#endif