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
        class GY_CreateConnTask;
        class GY_RecvTask;
        class GY_SendTask;
        class GY_Controller;

        class GY_Objector
        {
        public:
            using wptr = std::weak_ptr<GY_Objector>;
            using ptr = std::shared_ptr<GY_Objector>;
            using uptr = std::unique_ptr<GY_Objector>;
            GY_Objector();
            virtual void SetEventType(int event_type) = 0;
            virtual void ExecuteTask() = 0;
            virtual ~GY_Objector();
        };

        class Timer
        {
        public:
            using ptr = std::shared_ptr<Timer>;
            Timer(uint64_t timerid, uint64_t during_time, uint32_t exec_times, std::function<std::any()> &&func);
            // 获取当前时间戳
            static uint64_t GetCurrentTime();
            // 获取需要等待的时间
            uint64_t GetDuringTime();
            // 获取过期时间
            uint64_t GetExpiredTime();
            // 获取剩余时间
            uint64_t GetRemainTime();
            // 获取 timerid
            uint64_t GetTimerid();
            // 设置等待时间
            void SetDuringTime(uint64_t during_time);
            // 获取剩余需要执行的次数
            uint32_t &GetRemainExecTimes();
            // 执行任务
            void Execute();
            // 获取结果
            std::any Result();
            // 取消任务
            void Cancle();
            // 判断任务是否取消
            bool IsCancled();
            // 判断任务是否完成
            bool IsFinish();

        protected:
            uint64_t m_timerid;
            // the times to Exec timer
            uint32_t m_exec_times;
            uint64_t m_expired_time;
            uint64_t m_during_time;
            std::atomic_bool m_cancle;
            std::atomic_bool m_is_finish;
            std::function<std::any()> m_userfunc;
            std::any m_result;
        };

        class GY_TimerManager : public GY_Objector
        {
        public:
            using ptr = std::shared_ptr<GY_TimerManager>;
            using wptr = std::weak_ptr<GY_TimerManager>;
            using uptr = std::unique_ptr<GY_TimerManager>;
            GY_TimerManager();
            virtual void SetEventType(int event_type) override;
            virtual void ExecuteTask() override;
            Timer::ptr AddTimer(uint64_t during, uint32_t exec_times, std::function<std::any()> &&func);
            // return timerfd
            int GetTimerfd();
            ~GY_TimerManager();

        private:
            Timer::ptr GetEaliestTimer();

            // 添加或者执行完Timer，更新fd下一次触发的时间
            void UpdateTimerfd();

            class MyCompare
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
                    return a->GetTimerid() > b->GetTimerid();
                }
            };

        private:
            int m_timerfd;
            std::shared_mutex m_mtx;
            static std::atomic_uint64_t m_global_timerid; // 小于4,000,000,000
            std::priority_queue<Timer::ptr, std::vector<Timer::ptr>, MyCompare> m_timers;
        };

        class GY_Acceptor : public GY_Objector
        {
        public:
            using wptr = std::weak_ptr<GY_Acceptor>;
            using ptr = std::shared_ptr<GY_Acceptor>;
            using uptr = std::unique_ptr<GY_Acceptor>;
            GY_Acceptor(std::weak_ptr<galay::kernel::GY_SIOManager> ioManager);
            int GetListenFd();
            virtual void SetEventType(int event_type) override;
            inline virtual void ExecuteTask() override;
            virtual ~GY_Acceptor();

        private:
            std::unique_ptr<GY_CreateConnTask> m_listentask;
        };

        class GY_Receiver : public GY_Objector
        {
        public:
            using wptr = std::weak_ptr<GY_Receiver>;
            using ptr = std::shared_ptr<GY_Receiver>;
            using uptr = std::unique_ptr<GY_Receiver>;
            // 创建时既需要加入scheduler的事件中
            GY_Receiver(int fd, SSL* ssl, std::weak_ptr<galay::kernel::GY_IOScheduler> ioManager);
            std::string &GetRBuffer();
            virtual void SetEventType(int event_type) override;
            virtual void ExecuteTask() override;
            virtual ~GY_Receiver() = default;

        private:
            std::unique_ptr<GY_RecvTask> m_recvTask;
        };

        class GY_Sender : public GY_Objector
        {
        public:
            using ptr = std::shared_ptr<GY_Sender>;
            using wptr = std::weak_ptr<GY_Sender>;
            using uptr = std::unique_ptr<GY_Sender>;
            GY_Sender(int fd, SSL* ssl, std::weak_ptr<galay::kernel::GY_IOScheduler> scheduler);
            void AppendWBuffer(std::string &&wbuffer);
            bool WBufferEmpty();
            virtual void SetEventType(int event_type) override;
            virtual void ExecuteTask() override;
            virtual ~GY_Sender() = default;

        private:
            std::unique_ptr<GY_SendTask> m_sendTask;
        };

        class GY_Connector : public GY_Objector, public std::enable_shared_from_this<GY_Connector>
        {
        public:
            using ptr = std::shared_ptr<GY_Connector>;
            using wptr = std::weak_ptr<GY_Connector>;
            using uptr = std::unique_ptr<GY_Connector>;
            GY_Connector(int fd, SSL* ssl, std::weak_ptr<galay::kernel::GY_SIOManager> ioManager);
            void SetContext(std::any &&context);
            void PushResponse(std::string &&response);
            virtual void SetEventType(int event_type) override;
            std::shared_ptr<galay::kernel::Timer> AddTimer(uint64_t during, uint32_t exec_times, std::function<std::any()> &&func);
            std::any &&GetContext();
            std::weak_ptr<common::GY_NetCoroutinePool> GetCoPool();
            galay::protocol::GY_Request::ptr GetRequest();
           
            virtual void ExecuteTask() override;
            virtual ~GY_Connector();
        private:
            galay::common::GY_NetCoroutine<galay::common::CoroutineStatus> CoBusinessExec();
            galay::common::GY_NetCoroutine<galay::common::CoroutineStatus> CoReceiveExec();
            galay::common::GY_NetCoroutine<galay::common::CoroutineStatus> CoSendExec();

            void PushRequest(galay::protocol::GY_Request::ptr request);
            void RealSend();
            void RealRecv();
        private:
            int m_fd;
            SSL *m_ssl;
            std::weak_ptr<galay::kernel::GY_SIOManager> m_ioManager;
            int m_eventType;
            std::shared_ptr<galay::kernel::GY_Controller> m_controller;
            GY_Receiver::uptr m_receiver;
            GY_Sender::uptr m_sender;
            uint64_t m_MainCoId;
            uint64_t m_RecvCoId;
            uint64_t m_SendCoId;
            uint64_t m_UserCoId;
            protocol::GY_Request::ptr m_tempRequest;
            std::queue<protocol::GY_Request::ptr> m_requests;
            std::queue<std::string> m_responses;
            std::any m_context;
            std::shared_ptr<bool> m_exit;
        };

        class GY_ClientExcutor: public GY_Objector
        {
        public:
            using ptr = std::shared_ptr<GY_ClientExcutor>;
            using wptr = std::weak_ptr<GY_ClientExcutor>;
            using uptr = std::unique_ptr<GY_ClientExcutor>;
            GY_ClientExcutor(std::function<void()>&& func);
            virtual void SetEventType(int event_type) override;
            virtual void ExecuteTask() override;
        private:
            std::function<void()> m_func;
        };
    }

} // namespace galay

#endif
