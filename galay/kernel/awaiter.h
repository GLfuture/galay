#ifndef GALAY_AWAIT_H
#define GALAY_AWAIT_H

#include <memory>
#include <coroutine>
#include <optional>
#include "co_task.h"

namespace galay
{

    //return int
    class Awaiter_Base
    {
    public:
        using ptr = std::shared_ptr<Awaiter_Base>;

        Awaiter_Base(Co_Task_Base::ptr task)
        {
            this->m_task = task;
        }

        Awaiter_Base &operator=(const Awaiter_Base &) = delete;
        Awaiter_Base &operator=(Awaiter_Base &&other)
        {
            if(this != &other)
            {
                this->m_task = std::move(other.m_task);
                other.m_task = nullptr;
            }
            return *this;
        }

        virtual bool await_ready()
        {
            return m_task == nullptr;
        }

        virtual void await_suspend(std::coroutine_handle<> handle)
        {
            m_task->set_co_handle(handle);
        }

        std::any await_resume(){
            if(m_task) this->m_result = m_task->result();
            return m_result;
        }

    protected:
        Co_Task_Base::ptr m_task = nullptr;
        std::any m_result;
    };
    
    class Net_Awaiter: public Awaiter_Base
    {
    public:
        using ptr = std::shared_ptr<Net_Awaiter>;
        Net_Awaiter(Co_Task_Base::ptr task)
            : Awaiter_Base(task)
        {

        }

        Net_Awaiter(Co_Task_Base::ptr task, const std::any& result)
            :Awaiter_Base(task)
        {
            this->m_result = result;
        }
    };
}

#endif