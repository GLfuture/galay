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
#include <openssl/ssl.h>
#include "../common/base.h"
#include "../common/result.h"
#include "../util/io.h"
#include "controller.h"

namespace galay::coroutine
{
    class NetCoroutine;
    class NetCoroutinePool;
}

namespace galay::poller
{
#define MAX_TIMERID 40, 000, 000, 000

    enum SchedulerType
    {
        kSelectScheduler, // select
        kEpollScheduler,  // epoll
    };

    enum EventType
    {
        kEventRead = 0x1,
        kEventWrite = 0x2,
        kEventError = 0x4,
        kEventEpollET = 0x8,       // epoll et模式
        kEventEpollOneShot = 0x16, // epoll oneshot模式
    };

    class Callback
    {
    public:
        Callback& operator+=(const std::function<void()>& callback);
        void operator()();
    private:
        std::function<void()> m_callback;
    };

    class Objector
    {
    public:
        using wptr = std::weak_ptr<Objector>;
        using ptr = std::shared_ptr<Objector>;
        using uptr = std::unique_ptr<Objector>;
        virtual Callback& OnRead() = 0;
        virtual Callback& OnWrite() = 0;
    };

    class Timer: public std::enable_shared_from_this<Timer>
    {
    public:
        using ptr = std::shared_ptr<Timer>;
        Timer(uint64_t timerid, uint64_t during_time, uint32_t exec_times, std::function<void(Timer::ptr)> &&func);
        // 获取当前时间戳
        static uint64_t GetCurrentTime();
        // 获取需要等待的时间
        uint64_t GetDuringTime();
        // 获取过期时间
        uint64_t GetExpiredTime();
        // 获取剩余时间
        uint64_t GetRemainTime();
        // 获取 timerid
        uint64_t GetTimerId();
        // 设置等待时间
        void SetDuringTime(uint64_t during_time);
        // 获取剩余需要执行的次数
        uint32_t &GetRemainExecTimes();
        // 执行任务
        void Execute();
        // 取消任务
        void Cancle();
        // 判断任务是否取消
        bool IsCancled();
        // 判断任务是否完成
        bool Success();

    protected:
        uint64_t m_timerid;
        // the times to Exec timer
        uint32_t m_execTimes;
        uint64_t m_expiredTime;
        uint64_t m_duringTime;
        std::atomic_bool m_cancle;
        std::atomic_bool m_success;
        std::function<void(Timer::ptr)> m_rightHandle;
    };

    class TimerCompare
    {
    public:
        bool operator()(const Timer::ptr &a, const Timer::ptr &b);
    };
    
    class TimerManager: public Objector
    {
    public:
        using ptr = std::shared_ptr<TimerManager>;
        using wptr = std::weak_ptr<TimerManager>;
        using uptr = std::unique_ptr<TimerManager>;
        TimerManager();
        virtual Callback& OnRead() override;
        virtual Callback& OnWrite() override;
        Timer::ptr AddTimer(uint64_t during, uint32_t exec_times, std::function<void(Timer::ptr)> &&func);
        // return timerfd
        int GetTimerfd();
        ~TimerManager();
    private:
        Timer::ptr GetEaliestTimer();
        // 添加或者执行完Timer，更新fd下一次触发的时间
        void UpdateTimerfd();
    private:
        int m_timerfd;
        std::shared_mutex m_mtx;
        Callback m_readCallback;
        Callback m_sendCallback;
        static std::atomic_uint64_t m_global_timerid; // 小于4,000,000,000
        std::priority_queue<Timer::ptr, std::vector<Timer::ptr>, TimerCompare> m_timers;
    };

    class TcpAcceptor: public Objector
    {
    public:
        using wptr = std::weak_ptr<TcpAcceptor>;
        using ptr = std::shared_ptr<TcpAcceptor>;
        using uptr = std::unique_ptr<TcpAcceptor>;
        TcpAcceptor(std::weak_ptr<IOScheduler> manager, uint16_t port, uint16_t backlog, std::string requestName, io::net::SSLConfig::ptr config);
        int GetListenFd();
        virtual Callback& OnRead() override;
        virtual Callback& OnWrite() override;
        virtual ~TcpAcceptor();

    private:
        SSL_CTX *m_sslCtx;
        int m_listenFd;
        std::string m_errMsg;
        Callback m_readCallback;
        Callback m_sendCallback;
    };

    class TcpConnector: public Objector, public std::enable_shared_from_this<TcpConnector>
    {
    public:
        using ptr = std::shared_ptr<TcpConnector>;
        using wptr = std::weak_ptr<TcpConnector>;
        using uptr = std::unique_ptr<TcpConnector>;
        TcpConnector(int fd, SSL* ssl, std::weak_ptr<IOScheduler> scheduler, std::string requestName);
        void Close();
        std::shared_ptr<result::ResultInterface> Send(std::string &&response);
        std::shared_ptr<Timer> AddTimer(uint64_t during, uint32_t exec_times, std::function<void(std::shared_ptr<Timer>)> &&func);
        galay::protocol::Request::ptr GetRequest();
        void PopRequest();
        bool HasRequest();
        virtual Callback& OnRead() override;
        virtual Callback& OnWrite() override;
        virtual ~TcpConnector();
    private:
        void RealSend(std::shared_ptr<result::ResultInterface> result);
        void RealRecv();
    private:
        int m_fd;
        SSL *m_ssl;
        std::string m_wbuffer;
        std::string m_rbuffer;
        std::string m_requestName;
        std::atomic_bool m_isClosed;
        protocol::Request::ptr m_tempRequest;
        std::weak_ptr<IOScheduler> m_scheduler;
        std::queue<protocol::Request::ptr> m_requests;
        std::shared_ptr<server::Controller> m_controller;
        Callback m_readCallback;
        Callback m_sendCallback;
    };

    class ClientExcutor: public Objector
    {
    public:
        using ptr = std::shared_ptr<ClientExcutor>;
        using wptr = std::weak_ptr<ClientExcutor>;
        using uptr = std::unique_ptr<ClientExcutor>;
        ClientExcutor() = default;
        virtual Callback& OnRead() override;
        virtual Callback& OnWrite() override;
    private:
        Callback m_readCallback;
        Callback m_sendCallback;
    };

    class IOScheduler
    {
    public:
        using ptr = std::shared_ptr<IOScheduler>;
        using wptr = std::weak_ptr<IOScheduler>;
        using uptr = std::unique_ptr<IOScheduler>;
        IOScheduler() = default;
        virtual void Start() = 0;
        virtual void Stop() = 0;
        
        virtual void RegisterObjector(int fd, Objector::ptr objector) = 0;
        virtual void RegiserTimerManager(int fd, TimerManager::ptr timerManager) = 0;
        virtual void DelObjector(int fd) = 0;

        virtual int DelEvent(int fd, int event_type) = 0;
        virtual int ModEvent(int fd, int from, int to) = 0;
        virtual int AddEvent(int fd, int event_type) = 0;
        virtual TimerManager::ptr GetTimerManager() = 0;
        virtual ~IOScheduler() = default;
    };

    class SelectScheduler : public IOScheduler
    {
    public:
        using ptr = std::shared_ptr<SelectScheduler>;
        using wptr = std::weak_ptr<SelectScheduler>;
        using uptr = std::unique_ptr<SelectScheduler>;
        
        SelectScheduler(uint64_t timeout);

        virtual void RegisterObjector(int fd, Objector::ptr objector) override;
        virtual void RegiserTimerManager(int fd, TimerManager::ptr timerManager) override;
        virtual void DelObjector(int fd) override;

        virtual int DelEvent(int fd, int event_type) override;
        virtual int ModEvent(int fd, int from, int to) override;
        virtual int AddEvent(int fd, int event_type) override;

        virtual void Start() override;
        virtual void Stop() override;

        virtual TimerManager::ptr GetTimerManager() override;
        virtual ~SelectScheduler();
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
        std::unordered_map<int, Objector::ptr> m_objectors;
        std::queue<int> m_eraseQueue;
    };

    class EpollScheduler : public IOScheduler
    {
    public:
        using ptr = std::shared_ptr<EpollScheduler>;
        using uptr = std::unique_ptr<EpollScheduler>;
        using wptr = std::weak_ptr<EpollScheduler>;

        EpollScheduler(uint32_t eventSize, uint64_t timeout);
        virtual void RegisterObjector(int fd, Objector::ptr objector) override;
        virtual void RegiserTimerManager(int fd, TimerManager::ptr timerManager) override;
        virtual void DelObjector(int fd) override;
        virtual int DelEvent(int fd, int event_type) override;
        virtual int ModEvent(int fd, int from, int to) override;
        virtual int AddEvent(int fd, int event_type) override;

        virtual void Start() override;
        virtual void Stop() override;

        virtual TimerManager::ptr GetTimerManager() override;
        virtual ~EpollScheduler();
    private:
        bool RealDelObjector(int fd);
    private:
        bool m_stop;
        int m_epollfd;
        int m_timerfd;
        uint64_t m_timeout;
        epoll_event *m_events;
        uint32_t m_eventSize;
        std::unordered_map<int, Objector::ptr> m_objectors;
        std::queue<int> m_eraseQueue;

    };

    extern void SetWrongHandle(std::function<void(server::Controller::ptr)> handle);
    extern void SetRightHandle(std::function<void(server::Controller::ptr)> handle);

    extern int RecvAll(IOScheduler::ptr scheduler, int fd, SSL* ssl, std::string& recvBuffer);
    extern int SendAll(IOScheduler::ptr scheduler, int fd, SSL* ssl, std::string& buffer);

}


#endif