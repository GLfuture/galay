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
#include "../common/base.h"


namespace galay
{
    namespace coroutine
    {
        class GY_NetCoroutine;
        class GY_NetCoroutinePool;
    }

    namespace server
    {
        class GY_TcpServerBuilderBase;
        class GY_Controller;
    }

    namespace objector
    {
        class GY_Objector;
        class Timer;
        class GY_TimerManager;
    }

    namespace poller
    {
#define MAX_TIMERID 40, 000, 000, 000

        enum EventType
        {
            kEventRead = 0x1,
            kEventWrite = 0x2,
            kEventError = 0x4,
            kEventEpollET = 0x8,       // epoll et模式
            kEventEpollOneShot = 0x16, // epoll oneshot模式
        };

        

        class GY_IOScheduler
        {
        public:
            using ptr = std::shared_ptr<GY_IOScheduler>;
            using wptr = std::weak_ptr<GY_IOScheduler>;
            using uptr = std::unique_ptr<GY_IOScheduler>;
            GY_IOScheduler() = default;
            virtual void Start() = 0;
            virtual void Stop() = 0;
            
            virtual void RegisterObjector(int fd, std::shared_ptr<objector::GY_Objector> objector) = 0;
            virtual void RegiserTimerManager(int fd, std::shared_ptr<objector::GY_TimerManager> timerManager) = 0;
            virtual void DelObjector(int fd) = 0;

            virtual int DelEvent(int fd, int event_type) = 0;
            virtual int ModEvent(int fd, int from, int to) = 0;
            virtual int AddEvent(int fd, int event_type) = 0;
            virtual std::shared_ptr<objector::Timer> AddTimer(uint64_t during, uint32_t exec_times, std::function<void(std::shared_ptr<objector::Timer>)> &&func) = 0;
            virtual ~GY_IOScheduler() = default;
        };

        class GY_SelectScheduler : public GY_IOScheduler
        {
        public:
            using ptr = std::shared_ptr<GY_SelectScheduler>;
            using wptr = std::weak_ptr<GY_SelectScheduler>;
            using uptr = std::unique_ptr<GY_SelectScheduler>;
            
            GY_SelectScheduler(uint64_t timeout);

            virtual void RegisterObjector(int fd, std::shared_ptr<objector::GY_Objector> objector) override;
            virtual void RegiserTimerManager(int fd, std::shared_ptr<objector::GY_TimerManager> timerManager) override;
            virtual void DelObjector(int fd) override;

            virtual int DelEvent(int fd, int event_type) override;
            virtual int ModEvent(int fd, int from, int to) override;
            virtual int AddEvent(int fd, int event_type) override;

            virtual void Start() override;
            virtual void Stop() override;

            virtual std::shared_ptr<objector::Timer> AddTimer(uint64_t during, uint32_t exec_times, std::function<void(std::shared_ptr<objector::Timer>)> &&func) override;
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
            std::unordered_map<int, std::shared_ptr<objector::GY_Objector>> m_objectors;
            std::queue<int> m_eraseQueue;
        };

        class GY_EpollScheduler : public GY_IOScheduler
        {
        public:
            using ptr = std::shared_ptr<GY_EpollScheduler>;
            using uptr = std::unique_ptr<GY_EpollScheduler>;
            using wptr = std::weak_ptr<GY_EpollScheduler>;

            GY_EpollScheduler(uint32_t eventSize, uint64_t timeout);
            virtual void RegisterObjector(int fd, std::shared_ptr<objector::GY_Objector> objector) override;
            virtual void RegiserTimerManager(int fd, std::shared_ptr<objector::GY_TimerManager> timerManager) override;
            virtual void DelObjector(int fd) override;
            virtual int DelEvent(int fd, int event_type) override;
            virtual int ModEvent(int fd, int from, int to) override;
            virtual int AddEvent(int fd, int event_type) override;
            virtual std::shared_ptr<objector::Timer> AddTimer(uint64_t during, uint32_t exec_times, std::function<void(std::shared_ptr<objector::Timer>)> &&func) override;
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
            std::unordered_map<int, std::shared_ptr<objector::GY_Objector>> m_objectors;
            std::queue<int> m_eraseQueue;

        };
    }
}

#endif