#ifndef GALAY_AWAIT_H
#define GALAY_AWAIT_H

#include <memory>
#include <coroutine>
#include <optional>
#include "task_await.h"

namespace galay
{
    template<Request REQ , Response RESP , typename RESULT>
    class Awaiter_Base
    {
    public:
        using ptr = std::shared_ptr<Awaiter_Base>;

        Awaiter_Base(Co_Task_Base<REQ,RESP,RESULT>::ptr task)
        {
            this->m_task = task;
        }

        Awaiter_Base &operator=(const Awaiter_Base &) = delete;
        Awaiter_Base &operator=(Awaiter_Base<REQ,RESP,RESULT> &&other)
        {
            if(this != &other)
            {
                this->m_task = std::move(other.m_task);
                other.m_task = nullptr;
            }
        }

        virtual bool await_ready()
        {
            return m_task == nullptr;
        }

        virtual void await_suspend(std::coroutine_handle<> handle)
        {
            m_task->set_co_handle(handle);
        }

        RESULT await_resume(){
            if(m_task) m_result = m_task->result();
            if constexpr (!std::is_same_v<RESULT,void>)
                return std::move(m_result.value());
        }

    protected:
        Co_Task_Base<REQ,RESP,RESULT>::ptr m_task = nullptr;
        std::optional<RESULT> m_result;
    };
    
    template<Request REQ , Response RESP , typename RESULT>
    class Net_Awaiter: public Awaiter_Base<REQ,RESP,RESULT>
    {
    public:
        using ptr = std::shared_ptr<Net_Awaiter>;
        Net_Awaiter(Co_Task_Base<REQ,RESP,RESULT>::ptr task)
            : Awaiter_Base<REQ,RESP,RESULT>(task)
        {

        }

        Net_Awaiter(Co_Task_Base<REQ,RESP,RESULT>::ptr task, RESULT result)
            :Awaiter_Base<REQ,RESP,RESULT>(task)
        {
            this->m_result = result;
        }
        
    };
}

#endif