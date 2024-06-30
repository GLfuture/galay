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
        class GY_ThreadCond;

        class GY_IOScheduler
        {
        public:
            using ptr = std::shared_ptr<GY_IOScheduler>;
            using wptr = std::weak_ptr<GY_IOScheduler>;
            using uptr = std::unique_ptr<GY_IOScheduler>;
            GY_IOScheduler() = default;
            virtual GY_TcpServerBuilderBase::wptr GetTcpServerBuilder() = 0;
            virtual void RegisterObjector(int fd, std::shared_ptr<GY_Objector> objector) = 0;
            virtual void RegiserTimerManager(int fd, std::shared_ptr<GY_TimerManager> timerManager) = 0;
            virtual std::shared_ptr<Timer> AddTimer(uint64_t during, uint32_t exec_times, std::function<std::any()> &&func) = 0;
            virtual void DelObjector(int fd) = 0;
            virtual void Start() = 0;
            virtual common::GY_TcpCoroutine<common::CoroutineStatus> UserFunction(GY_Controller::ptr controller) = 0;
            virtual common::GY_TcpCoroutine<common::CoroutineStatus> IllegalFunction(std::string &rbuffer, std::string &wbuffer) = 0;
            virtual int DelEvent(int fd, int event_type) = 0;
            virtual int ModEvent(int fd, int from, int to) = 0;
            virtual int AddEvent(int fd, int event_type) = 0;
            virtual void Stop() = 0;
            virtual bool IsStop() = 0;
            virtual ~GY_IOScheduler() = default;

        protected:
            std::unordered_map<int, std::shared_ptr<GY_Objector>> m_objectors;
        };
        

        class GY_SelectScheduler : public GY_IOScheduler
        {
        public:
            using ptr = std::shared_ptr<GY_SelectScheduler>;
            using wptr = std::weak_ptr<GY_SelectScheduler>;
            using uptr = std::unique_ptr<GY_SelectScheduler>;
            GY_SelectScheduler(GY_TcpServerBuilderBase::ptr builder,std::shared_ptr<GY_ThreadCond> threadCond);
            virtual GY_TcpServerBuilderBase::wptr GetTcpServerBuilder() override;
            virtual void RegisterObjector(int fd, std::shared_ptr<GY_Objector> objector) override;
            virtual void RegiserTimerManager(int fd, std::shared_ptr<GY_TimerManager> timerManager) override;
            virtual void DelObjector(int fd) override;
            virtual std::shared_ptr<Timer> AddTimer(uint64_t during, uint32_t exec_times, std::function<std::any()> &&func) override;
            virtual void Start() override;
            virtual common::GY_TcpCoroutine<common::CoroutineStatus> UserFunction(GY_Controller::ptr controller) override;
            virtual common::GY_TcpCoroutine<common::CoroutineStatus> IllegalFunction(std::string &rbuffer, std::string &wbuffer) override;
            virtual int DelEvent(int fd, int event_type) override;
            virtual int ModEvent(int fd, int from, int to) override;
            virtual int AddEvent(int fd, int event_type) override;
            virtual bool IsStop() override;
            virtual void Stop() override;
            virtual ~GY_SelectScheduler();

        private:
            int m_timerfd;
            fd_set m_rfds;
            fd_set m_wfds;
            fd_set m_efds;
            int m_maxfd = INT32_MIN;
            int m_minfd = INT32_MAX;
            std::atomic_bool m_stop;
            GY_TcpServerBuilderBase::ptr m_builder;
            std::shared_ptr<GY_ThreadCond> m_threadCond;
            std::function<common::GY_TcpCoroutine<common::CoroutineStatus>(GY_Controller::wptr)> m_userFunc;
            std::function<common::GY_TcpCoroutine<common::CoroutineStatus>(std::string &, std::string &)> m_illegalFunc;
        };

        class GY_EpollScheduler : public GY_IOScheduler
        {
        public:
            using ptr = std::shared_ptr<GY_EpollScheduler>;
            using uptr = std::unique_ptr<GY_EpollScheduler>;
            using wptr = std::weak_ptr<GY_EpollScheduler>;

            GY_EpollScheduler(GY_TcpServerBuilderBase::ptr builder,std::shared_ptr<GY_ThreadCond> threadCond);
            virtual GY_TcpServerBuilderBase::wptr GetTcpServerBuilder() override;
            virtual void RegisterObjector(int fd, std::shared_ptr<GY_Objector> objector) override;
            virtual void RegiserTimerManager(int fd, std::shared_ptr<GY_TimerManager> timerManager) override;
            virtual std::shared_ptr<Timer> AddTimer(uint64_t during, uint32_t exec_times, std::function<std::any()> &&func) override;
            virtual void DelObjector(int fd) override;
            virtual void Start() override;
            virtual common::GY_TcpCoroutine<common::CoroutineStatus> UserFunction(GY_Controller::ptr controller) override;
            virtual common::GY_TcpCoroutine<common::CoroutineStatus> IllegalFunction(std::string &rbuffer, std::string &wbuffer) override;
            virtual int DelEvent(int fd, int event_type) override;
            virtual int ModEvent(int fd, int from, int to) override;
            virtual int AddEvent(int fd, int event_type) override;
            virtual void Stop() override;
            virtual bool IsStop() override;
            virtual ~GY_EpollScheduler();

        private:
            int m_epollfd;
            int m_timerfd;
            GY_TcpServerBuilderBase::ptr m_builder;
            std::shared_ptr<GY_ThreadCond> m_threadCond;
            epoll_event *m_events;
            std::atomic_bool m_stop;
            std::function<common::GY_TcpCoroutine<common::CoroutineStatus>(GY_Controller::wptr)> m_userFunc;
            std::function<common::GY_TcpCoroutine<common::CoroutineStatus>(std::string &, std::string &)> m_illegalFunc;
        };

    }
}

#endif