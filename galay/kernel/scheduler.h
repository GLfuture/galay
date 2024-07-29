#ifndef GALAY_SCHEDULER_H
#define GALAY_SCHEDULER_H

#include <memory>
#include <atomic>
#include <queue>
#include <chrono>
#include <memory>
#include <mutex>
#include <functional>
#include <shared_mutex>
#include <unordered_map>
#include <sys/timerfd.h>
#include <sys/epoll.h> 

#include "builder.h"

namespace galay
{
    namespace common
    {
        class GY_NetCoroutinePool;
    }

    namespace kernel
    {
#define MAX_TIMERID 40, 000, 000, 000

        enum EventType
        {
            kEventRead = 0x1,
            kEventWrite = 0x2,
            kEventError = 0x4,
            kEvnetEpollET = 0x8,       // epoll et模式
            kEventEpollOneShot = 0x16, // epoll oneshot模式
        };

        class GY_Objector;
        class Timer;
        class GY_TimerManager;

        class GY_IOScheduler
        {
        public:
            using ptr = std::shared_ptr<GY_IOScheduler>;
            using wptr = std::weak_ptr<GY_IOScheduler>;
            using uptr = std::unique_ptr<GY_IOScheduler>;
            GY_IOScheduler() = default;
            virtual void Start() = 0;
            virtual void Stop() = 0;
            
            virtual void RegisterObjector(int fd, std::shared_ptr<GY_Objector> objector) = 0;
            virtual void RegiserTimerManager(int fd, std::shared_ptr<GY_TimerManager> timerManager) = 0;
            virtual void DelObjector(int fd) = 0;

            virtual int DelEvent(int fd, int event_type) = 0;
            virtual int ModEvent(int fd, int from, int to) = 0;
            virtual int AddEvent(int fd, int event_type) = 0;
            virtual std::shared_ptr<Timer> AddTimer(uint64_t during, uint32_t exec_times, std::function<std::any()> &&func) = 0;
            virtual ~GY_IOScheduler() = default;

        protected:
            std::unordered_map<int, std::shared_ptr<GY_Objector>> m_objectors;
            std::queue<int> m_eraseQueue;
        };

        class GY_SelectScheduler : public GY_IOScheduler
        {
        public:
            using ptr = std::shared_ptr<GY_SelectScheduler>;
            using wptr = std::weak_ptr<GY_SelectScheduler>;
            using uptr = std::unique_ptr<GY_SelectScheduler>;
            
            GY_SelectScheduler(uint64_t timeout);

            virtual void RegisterObjector(int fd, std::shared_ptr<GY_Objector> objector) override;
            virtual void RegiserTimerManager(int fd, std::shared_ptr<GY_TimerManager> timerManager) override;
            virtual void DelObjector(int fd) override;

            virtual int DelEvent(int fd, int event_type) override;
            virtual int ModEvent(int fd, int from, int to) override;
            virtual int AddEvent(int fd, int event_type) override;

            virtual void Start() override;
            virtual void Stop() override;

            virtual std::shared_ptr<Timer> AddTimer(uint64_t during, uint32_t exec_times, std::function<std::any()> &&func) override;
            virtual ~GY_SelectScheduler();
        private:
            bool RealDelObjector(int fd);
        private:
            uint64_t m_timeout;
            int m_timerfd;
            fd_set m_rfds;
            fd_set m_wfds;
            fd_set m_efds;
            int m_maxfd = INT32_MIN;
            int m_minfd = INT32_MAX;
            std::atomic_bool m_stop;
            std::unordered_map<int, std::shared_ptr<GY_Objector>> m_objectors;
            std::queue<int> m_eraseQueue;
        };

        class GY_EpollScheduler : public GY_IOScheduler
        {
        public:
            using ptr = std::shared_ptr<GY_EpollScheduler>;
            using uptr = std::unique_ptr<GY_EpollScheduler>;
            using wptr = std::weak_ptr<GY_EpollScheduler>;

            GY_EpollScheduler(uint32_t eventSize, uint64_t timeout);
            virtual void RegisterObjector(int fd, std::shared_ptr<GY_Objector> objector) override;
            virtual void RegiserTimerManager(int fd, std::shared_ptr<GY_TimerManager> timerManager) override;
            virtual void DelObjector(int fd) override;
            virtual int DelEvent(int fd, int event_type) override;
            virtual int ModEvent(int fd, int from, int to) override;
            virtual int AddEvent(int fd, int event_type) override;
            virtual std::shared_ptr<Timer> AddTimer(uint64_t during, uint32_t exec_times, std::function<std::any()> &&func) override;
            virtual void Start() override;
            virtual void Stop() override;

            virtual ~GY_EpollScheduler();
        private:
            bool RealDelObjector(int fd);
        private:
            bool m_stop;
            int m_epollfd;
            int m_timerfd;
            uint64_t m_timeout;
            epoll_event *m_events;
            uint32_t m_eventSize;
            std::unordered_map<int, std::shared_ptr<GY_Objector>> m_objectors;
            std::queue<int> m_eraseQueue;

        };

        class GY_SIOManager
        {
        public:
            using ptr = std::shared_ptr<GY_SIOManager>;
            using wptr = std::weak_ptr<GY_SIOManager>;
            using uptr = std::unique_ptr<GY_SIOManager>;

            GY_SIOManager(GY_TcpServerBuilderBase::ptr builder);
            virtual void Start();
            virtual void Stop();
            virtual GY_IOScheduler::ptr GetIOScheduler();
            virtual GY_TcpServerBuilderBase::wptr GetTcpServerBuilder();
            virtual std::shared_ptr<common::GY_NetCoroutinePool> GetCoroutinePool();
            virtual common::GY_NetCoroutine<common::CoroutineStatus> UserFunction(GY_Controller::ptr controller);
            virtual std::string IllegalFunction();
            virtual ~GY_SIOManager();
        private:
            bool m_stop;
            GY_IOScheduler::ptr m_ioScheduler;
            GY_TcpServerBuilderBase::ptr m_builder;
            //协程相关
            std::shared_ptr<common::GY_NetCoroutinePool> m_coPool;
            //函数相关
            std::function<common::GY_NetCoroutine<common::CoroutineStatus>(GY_Controller::wptr)> m_userFunc;
            std::function<std::string()> m_illegalFunc;
        };
    }
}

#endif