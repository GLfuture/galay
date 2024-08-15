#ifndef GALAY_OBJECTOR_H
#define GALAY_OBJECTOR_H

#include <memory>
#include <atomic>
#include <functional>
#include <queue>
#include <shared_mutex>
#include <any>
#include <openssl/ssl.h>
#include "../common/awaiter.h"
#include "../common/base.h"


namespace galay
{
    namespace kernel
    {
        class GY_SIOManager;
        class GY_IOScheduler;
        class GY_TcpCreateConnTask;
        class GY_TcpRecvTask;
        class GY_TcpSendTask;
        class GY_Controller;
        class NetResult;

        class Callback
        {
        public:
            Callback& operator+=(const std::function<void()>& callback)
            {
                m_callback = callback;
                return *this;
            }

            void operator()()
            {
                m_callback();
            } 
        private:
            std::function<void()> m_callback;
        };

        class GY_Objector
        {
        public:
            using wptr = std::weak_ptr<GY_Objector>;
            using ptr = std::shared_ptr<GY_Objector>;
            using uptr = std::unique_ptr<GY_Objector>;
            GY_Objector();
            virtual Callback& OnRead() = 0;
            virtual Callback& OnWrite() = 0;
            virtual ~GY_Objector();
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
            std::function<void(Timer::ptr)> m_userfunc;
        };

        class GY_TimerManager: public GY_Objector
        {
        public:
            using ptr = std::shared_ptr<GY_TimerManager>;
            using wptr = std::weak_ptr<GY_TimerManager>;
            using uptr = std::unique_ptr<GY_TimerManager>;
            GY_TimerManager();
            virtual Callback& OnRead() override;
            virtual Callback& OnWrite() override;
            Timer::ptr AddTimer(uint64_t during, uint32_t exec_times, std::function<void(Timer::ptr)> &&func);
            // return timerfd
            int GetTimerfd();
            ~GY_TimerManager();
        private:
            Timer::ptr GetEaliestTimer();
            // 添加或者执行完Timer，更新fd下一次触发的时间
            void UpdateTimerfd();

            class TimerCompare
            {
            public:
                bool operator()(const Timer::ptr &a, const Timer::ptr &b)
                {
                    if (a->GetExpiredTime() > b->GetExpiredTime())
                    {
                        return true;
                    }
                    else if (a->GetExpiredTime() < b->GetExpiredTime())
                    {
                        return false;
                    }
                    return a->GetTimerId() > b->GetTimerId();
                }
            };

        private:
            int m_timerfd;
            std::shared_mutex m_mtx;
            Callback m_readCallback;
            Callback m_sendCallback;
            static std::atomic_uint64_t m_global_timerid; // 小于4,000,000,000
            std::priority_queue<Timer::ptr, std::vector<Timer::ptr>, TimerCompare> m_timers;
        };

        class GY_TcpAcceptor: public GY_Objector
        {
        public:
            using wptr = std::weak_ptr<GY_TcpAcceptor>;
            using ptr = std::shared_ptr<GY_TcpAcceptor>;
            using uptr = std::unique_ptr<GY_TcpAcceptor>;
            GY_TcpAcceptor(std::weak_ptr<galay::kernel::GY_SIOManager> ioManager);
            int GetListenFd();
            virtual Callback& OnRead() override;
            virtual Callback& OnWrite() override;
            virtual ~GY_TcpAcceptor();

        private:
            std::unique_ptr<GY_TcpCreateConnTask> m_listentask;
            Callback m_readCallback;
            Callback m_sendCallback;
        };

        class GY_TcpConnector: public GY_Objector, public std::enable_shared_from_this<GY_TcpConnector>
        {
        public:
            using ptr = std::shared_ptr<GY_TcpConnector>;
            using wptr = std::weak_ptr<GY_TcpConnector>;
            using uptr = std::unique_ptr<GY_TcpConnector>;
            GY_TcpConnector(int fd, SSL* ssl, std::weak_ptr<galay::kernel::GY_SIOManager> ioManager);
            void Close();
            void SetContext(std::any &&context);
            std::shared_ptr<NetResult> Send(std::string &&response);
            std::shared_ptr<galay::kernel::Timer> AddTimer(uint64_t during, uint32_t exec_times, std::function<void(std::shared_ptr<Timer>)> &&func);
            std::any &&GetContext();
            galay::protocol::GY_SRequest::ptr GetRequest();
            void PopRequest();
            bool HasRequest();
            virtual Callback& OnRead() override;
            virtual Callback& OnWrite() override;
            virtual ~GY_TcpConnector();
        private:
            void RealSend(std::shared_ptr<NetResult> result);
            void RealRecv();
        private:
            int m_fd;
            SSL *m_ssl;
            std::any m_context;
            std::atomic_bool m_isClosed;
            protocol::GY_SRequest::ptr m_tempRequest;
            std::unique_ptr<GY_TcpRecvTask> m_recvTask;
            std::unique_ptr<GY_TcpSendTask> m_sendTask;
            std::queue<protocol::GY_SRequest::ptr> m_requests;
            std::weak_ptr<galay::kernel::GY_SIOManager> m_ioManager;
            std::shared_ptr<galay::kernel::GY_Controller> m_controller;
            Callback m_readCallback;
            Callback m_sendCallback;
        };

        class GY_ClientExcutor: public GY_Objector
        {
        public:
            using ptr = std::shared_ptr<GY_ClientExcutor>;
            using wptr = std::weak_ptr<GY_ClientExcutor>;
            using uptr = std::unique_ptr<GY_ClientExcutor>;
            GY_ClientExcutor() = default;
            virtual Callback& OnRead() override;
            virtual Callback& OnWrite() override;
        private:
            Callback m_readCallback;
            Callback m_sendCallback;
        };
    }

} // namespace galay

#endif
