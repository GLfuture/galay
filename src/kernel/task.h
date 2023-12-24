#ifndef GALAY_TASK_H
#define GALAY_TASK_H

#include <functional>
#include <memory>
#include <string>
#include "iofunction.h"
#include "basic_concepts.h"


namespace galay
{
    template<Request REQ,Response RESP>
    class Task
    {
    public:
        using ptr = std::shared_ptr<Task>;
        virtual std::shared_ptr<REQ> get_req() = 0;
        virtual std::shared_ptr<RESP>  get_resp() = 0;
        virtual ~Task() {}
    };

    template<Request REQ,Response RESP>
    class Tcp_Task : public Task<REQ,RESP>
    {
    public:
        using ptr = std::shared_ptr<Tcp_Task>;
        Tcp_Task()
        {
            this->m_req = std::make_shared<REQ>();
            this->m_resp = std::make_shared<RESP>();
        }

        void Set_SSL(SSL *ssl) { m_ssl = ssl; }

        SSL *Get_SSL() { return m_ssl; }
       
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

        std::shared_ptr<REQ> get_req() override { return this->m_req; }
        std::shared_ptr<RESP>  get_resp()  override { return this->m_resp; }
        //return status
        int decode()
        {
            int state = error::protocol_error::GY_PROTOCOL_SUCCESS;
            int len = this->m_req->decode(this->m_rbuffer, state);
            if (state == error::protocol_error::GY_PROTOCOL_INCOMPLETE)
                return state;
            this->m_rbuffer.erase(this->m_rbuffer.begin(), this->m_rbuffer.begin() + len);
            return state;
        }

        void encode()
        {
            m_wbuffer = m_resp->encode();
        }

    protected:
        SSL *m_ssl = nullptr;
        //origin's dada
        std::string m_rbuffer;
        std::string m_wbuffer;
        std::shared_ptr<REQ> m_req;
        std::shared_ptr<RESP> m_resp;
    };

}

#endif