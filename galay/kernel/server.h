#ifndef GALAY_SERVER_H
#define GALAY_SERVER_H

#include "task.h"
#include <thread>

namespace galay
{
    namespace kernel
    {
        class GY_ThreadCond
        {
        public:
            using ptr = std::shared_ptr<GY_ThreadCond>;
            using wptr = std::weak_ptr<GY_ThreadCond>;
            using uptr = std::unique_ptr<GY_ThreadCond>;
            GY_ThreadCond(uint16_t threadNum);
            void WaitForThreads();
            void DecreaseThread();
        private:
            std::atomic_int16_t m_threadNum;
            std::mutex m_thMutex;
            std::condition_variable m_thCond;
        };

        class GY_Server
        {
        public:
            using ptr = std::shared_ptr<GY_Server>;
            using wptr = std::weak_ptr<GY_Server>;
            using uptr = std::unique_ptr<GY_Server>;
            virtual void Start() = 0;
            virtual void Stop() = 0;
            virtual GY_TcpServerBuilderBase::ptr GetServerBuilder() = 0;
            virtual void SetServerBuilder(kernel::GY_TcpServerBuilderBase::ptr builder) = 0;
            virtual GY_IOScheduler::wptr GetScheduler(int indx) = 0;
            virtual ~GY_Server() = default;
        };

        class GY_TcpServer : public GY_Server, public std::enable_shared_from_this<GY_TcpServer>
        {
        public:
            using ptr = std::shared_ptr<GY_TcpServer>;
            using wptr = std::weak_ptr<GY_TcpServer>;
            using uptr = std::unique_ptr<GY_TcpServer>;
            GY_TcpServer();
            virtual void Start() override;
            virtual void Stop() override;
            virtual GY_TcpServerBuilderBase::ptr GetServerBuilder() override;
            virtual void SetServerBuilder(GY_TcpServerBuilderBase::ptr builder) override;
            virtual GY_IOScheduler::wptr GetScheduler(int indx) override;

            virtual ~GY_TcpServer();
        private:
            GY_IOScheduler::ptr CreateScheduler();
            GY_TimerManager::ptr CreateTimerManager();
            GY_Acceptor::ptr CreateAcceptor(GY_IOScheduler::ptr scheduler);
        private:
            GY_TcpServerBuilderBase::ptr m_builder;
            std::vector<std::shared_ptr<std::thread>> m_threads;
            std::vector<GY_IOScheduler::ptr> m_schedulers;
            std::atomic_bool m_isStopped;
            GY_ThreadCond::ptr m_threadCond;
            std::condition_variable m_exitCond;
        };
    }
}

#endif