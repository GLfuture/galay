#ifndef GALAY_TASK_H
#define GALAY_TASK_H

#include <functional>
#include <memory>
#include <string>
#include "iofunction.h"
#include "context.h"

namespace galay
{
    class Task
    {
    public:
        using ptr = std::shared_ptr<Task>;
        virtual ~Task() {}
    };

    class Tcp_Task : public Task
    {
    public:
        using ptr = std::shared_ptr<Tcp_Task>;
        Tcp_Task()
        {
            this->m_ssl = nullptr;
            this->m_rbuffer = "";
            this->m_wbuffer = "";
        }

        size_t Get_Rbuffer_Length() { return m_rbuffer.length(); }

        size_t Get_Wbuffer_Length() { return m_wbuffer.length(); }

        void Set_SSL(SSL *ssl) { m_ssl = ssl; }

        SSL *Get_SSL()
        {
            return m_ssl;
        }
       
        std::string &Get_Rbuffer() { return m_rbuffer; }

        std::string &Get_Wbuffer() { return m_wbuffer; }

        virtual ~Tcp_Task()
        {
            if (m_ssl)
            {
                iofunction::Tcp_Function::SSL_Destory(m_ssl);
                m_ssl = nullptr;
            }
        }

    protected:
        SSL *m_ssl = nullptr;
        std::string m_rbuffer;
        std::string m_wbuffer;

    protected:
    };

}

#endif