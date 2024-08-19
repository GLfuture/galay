#ifndef GALAY_TASK_H
#define GALAY_TASK_H

#include "../util/netiofunction.h"

namespace galay
{
    namespace poller
    {
        class GY_IOScheduler;
    }

    namespace server
    {
        class GY_SIOManager;
        class GY_TcpServerBuilderBase;
    }

    namespace task
    {   
        class GY_Task
        {
        public:
            virtual void Execute() = 0;
            virtual bool Success() = 0;
            virtual std::string Error() = 0;
        };

        class GY_TcpCreateConnTask : public GY_Task
        {
        public:
            using ptr = std::shared_ptr<GY_TcpCreateConnTask>;
            using wptr = std::weak_ptr<GY_TcpCreateConnTask>;
            using uptr = std::unique_ptr<GY_TcpCreateConnTask>;
            GY_TcpCreateConnTask(std::weak_ptr<server::GY_SIOManager> scheduler);
            virtual void Execute() override;
            virtual bool Success() override;
            virtual std::string Error() override;
            int GetFd();
            virtual ~GY_TcpCreateConnTask() = default;

        protected:
            int CreateListenFd(std::weak_ptr<server::GY_TcpServerBuilderBase> builder);
            int CreateConn();
        protected:
            int m_fd;
            bool m_success;
            std::string m_error;
            std::weak_ptr<server::GY_SIOManager> m_ioManager;
        };

        class GY_TcpRecvTask : public GY_Task
        {
        public:
            using ptr = std::shared_ptr<GY_TcpRecvTask>;
            using wptr = std::weak_ptr<GY_TcpRecvTask>;
            using uptr = std::unique_ptr<GY_TcpRecvTask>;
            GY_TcpRecvTask(int fd, SSL* ssl, std::weak_ptr<poller::GY_IOScheduler> scheduler);
            void RecvAll();
            std::string &GetRBuffer();
            virtual bool Success() override;
            virtual std::string Error() override;
        protected:
            virtual void Execute() override;

        protected:
            int m_fd;
            SSL *m_ssl;
            bool m_success;
            std::string m_error;
            std::weak_ptr<poller::GY_IOScheduler> m_scheduler;
            std::string m_rbuffer;
        };

        class GY_TcpSendTask : public GY_Task
        {
        public:
            using ptr = std::shared_ptr<GY_TcpSendTask>;
            using wptr = std::weak_ptr<GY_TcpSendTask>;
            using uptr = std::unique_ptr<GY_TcpSendTask>;
            GY_TcpSendTask(int fd, SSL*ssl, std::weak_ptr<poller::GY_IOScheduler> scheduler);
            void AppendWBuffer(std::string &&wbuffer);
            void SendAll();
            bool Empty() const;
            virtual bool Success() override;
            virtual std::string Error() override;
        protected:
            virtual void Execute() override;

        protected:
            int m_fd;
            SSL *m_ssl;
            bool m_success;
            std::string m_error;
            std::string m_wbuffer;
            std::weak_ptr<poller::GY_IOScheduler> m_scheduler;
        };

        class GY_TcpCreateSSLConnTask: public GY_TcpCreateConnTask
        {
        public:
            using ptr = std::shared_ptr<GY_TcpCreateSSLConnTask>;
            using wptr = std::weak_ptr<GY_TcpCreateSSLConnTask>;
            using uptr = std::unique_ptr<GY_TcpCreateSSLConnTask>;

            GY_TcpCreateSSLConnTask(std::weak_ptr<server::GY_SIOManager> scheduler);
            virtual ~GY_TcpCreateSSLConnTask();
        protected:
            int CreateConn();
            virtual void Execute() override;
        protected:
            SSL_CTX *m_sslCtx;
        };
    }
}

#endif