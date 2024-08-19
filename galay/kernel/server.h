#ifndef GALAY_SERVER_H
#define GALAY_SERVER_H

#include "../common/threadpool.h"
#include <thread>

namespace galay
{
    namespace poller
    {
        class GY_IOScheduler;
    }
    namespace objector
    {
        class GY_TimerManager;
        class GY_TcpAcceptor;
    }

    namespace server
    {   
        class GY_Controller;
        class GY_TcpServerBuilderBase;
        
        class GY_SIOManager
        {
        public:
            using ptr = std::shared_ptr<GY_SIOManager>;
            using wptr = std::weak_ptr<GY_SIOManager>;
            using uptr = std::unique_ptr<GY_SIOManager>;

            GY_SIOManager(std::shared_ptr<server::GY_TcpServerBuilderBase> builder);
            virtual void Start();
            virtual void Stop();
            virtual std::shared_ptr<poller::GY_IOScheduler> GetIOScheduler();
            virtual std::weak_ptr<GY_TcpServerBuilderBase> GetTcpServerBuilder();
            virtual void RightHandle(std::shared_ptr<GY_Controller> controller);
            virtual void WrongHandle(std::shared_ptr<GY_Controller> controller);
            virtual ~GY_SIOManager();
        private:
            bool m_stop;
            std::shared_ptr<poller::GY_IOScheduler> m_ioScheduler;
            std::shared_ptr<GY_TcpServerBuilderBase> m_builder;
            //函数相关
            std::function<void(std::shared_ptr<GY_Controller>)> m_rightHandle;
            std::function<void(std::shared_ptr<GY_Controller>)> m_wrongHandle;
        };

        class GY_Server
        {
        public:
            using ptr = std::shared_ptr<GY_Server>;
            using wptr = std::weak_ptr<GY_Server>;
            using uptr = std::unique_ptr<GY_Server>;
            virtual void Start() = 0;
            virtual void Stop() = 0;
            virtual std::shared_ptr<GY_TcpServerBuilderBase> GetServerBuilder() = 0;
            virtual void SetServerBuilder(std::shared_ptr<GY_TcpServerBuilderBase> builder) = 0;
            virtual std::weak_ptr<GY_SIOManager> GetManager(int indx) = 0;
            virtual ~GY_Server() = default;
        };

        class GY_TcpServer : public GY_Server
        {
        public:
            using ptr = std::shared_ptr<GY_TcpServer>;
            using wptr = std::weak_ptr<GY_TcpServer>;
            using uptr = std::unique_ptr<GY_TcpServer>;
            GY_TcpServer();
            virtual void Start() override;
            virtual void Stop() override;
            virtual std::shared_ptr<GY_TcpServerBuilderBase> GetServerBuilder() override;
            virtual void SetServerBuilder(std::shared_ptr<GY_TcpServerBuilderBase> builder) override;
            virtual std::weak_ptr<GY_SIOManager> GetManager(int indx) override;

            virtual ~GY_TcpServer();
        private:
            std::shared_ptr<objector::GY_TimerManager> CreateTimerManager();
            std::shared_ptr<objector::GY_TcpAcceptor> CreateAcceptor(std::shared_ptr<GY_SIOManager> ioManager);
        private:
            std::shared_ptr<GY_TcpServerBuilderBase> m_builder;
            thread::GY_ThreadPool::ptr m_threadPool;
            std::vector<std::shared_ptr<GY_SIOManager>> m_ioManagers;
            std::atomic_bool m_isStopped;
        };
    }
}

#endif