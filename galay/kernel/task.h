#ifndef GALAY_TASK_H
#define GALAY_TASK_H

#include "../util/netiofunction.h"

namespace galay
{
    namespace kernel
    {   
        class GY_IOScheduler;
        class GY_SIOManager;
        class GY_TcpServerBuilderBase;

        class GY_Task
        {
        public:
            virtual void Execute() = 0;
            virtual bool Success() = 0;
            virtual std::string Error() = 0;
        };

        class GY_CreateConnTask : public GY_Task
        {
        public:
            using ptr = std::shared_ptr<GY_CreateConnTask>;
            using wptr = std::weak_ptr<GY_CreateConnTask>;
            using uptr = std::unique_ptr<GY_CreateConnTask>;
            GY_CreateConnTask(std::weak_ptr<GY_SIOManager> scheduler);
            virtual void Execute() override;
            virtual bool Success() override;
            virtual std::string Error() override;
            int GetFd();
            virtual ~GY_CreateConnTask() = default;

        protected:
            int CreateListenFd(std::weak_ptr<GY_TcpServerBuilderBase> builder);
            int CreateConn();
        protected:
            int m_fd;
            bool m_success;
            std::string m_error;
            std::weak_ptr<GY_SIOManager> m_ioManager;
        };

        class GY_RecvTask : public GY_Task
        {
        public:
            using ptr = std::shared_ptr<GY_RecvTask>;
            using wptr = std::weak_ptr<GY_RecvTask>;
            using uptr = std::unique_ptr<GY_RecvTask>;
            GY_RecvTask(int fd, SSL* ssl, std::weak_ptr<GY_IOScheduler> scheduler);
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
            std::weak_ptr<GY_IOScheduler> m_scheduler;
            std::string m_rbuffer;
        };

        class GY_SendTask : public GY_Task
        {
        public:
            using ptr = std::shared_ptr<GY_SendTask>;
            using wptr = std::weak_ptr<GY_SendTask>;
            using uptr = std::unique_ptr<GY_SendTask>;
            GY_SendTask(int fd, SSL*ssl, std::weak_ptr<GY_IOScheduler> scheduler);
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
            std::weak_ptr<GY_IOScheduler> m_scheduler;
        };

        class GY_CreateSSLConnTask: public GY_CreateConnTask
        {
        public:
            using ptr = std::shared_ptr<GY_CreateSSLConnTask>;
            using wptr = std::weak_ptr<GY_CreateSSLConnTask>;
            using uptr = std::unique_ptr<GY_CreateSSLConnTask>;

            GY_CreateSSLConnTask(std::weak_ptr<GY_SIOManager> scheduler);
            virtual ~GY_CreateSSLConnTask();
        protected:
            int CreateConn();
            virtual void Execute() override;
        protected:
            SSL_CTX *m_sslCtx;
        };
    }
}

#endif