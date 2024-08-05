#ifndef GALAY_SERVER_H
#define GALAY_SERVER_H

#include "../common/threadpool.h"
#include <thread>

namespace galay
{
    namespace kernel
    {   
        class GY_TcpServerBuilderBase;
        class GY_SIOManager;
        class GY_TimerManager;
        class GY_Acceptor;

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
            std::shared_ptr<GY_TimerManager> CreateTimerManager();
            std::shared_ptr<GY_Acceptor> CreateAcceptor(std::shared_ptr<GY_SIOManager> ioManager);
        private:
            std::shared_ptr<GY_TcpServerBuilderBase> m_builder;
            common::GY_ThreadPool::ptr m_threadPool;
            std::vector<std::shared_ptr<GY_SIOManager>> m_ioManagers;
            std::atomic_bool m_isStopped;
        };
    }
}

#endif