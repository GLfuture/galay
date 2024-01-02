#ifndef GALAY_AWAIT_H
#define GALAY_AWAIT_H

#include <memory>
#include <coroutine>
#include <optional>
#include "task.h"

namespace galay
{
    template<Request REQ,Response RESP>
    class Awaiter_Base
    {
    public:
        using ptr = std::shared_ptr<Awaiter_Base>;

        Awaiter_Base(Task<REQ,RESP>::ptr task)
        {
            this->m_task = task;
        }

        Awaiter_Base &operator=(const Awaiter_Base &) = delete;
        Awaiter_Base &operator=(Awaiter_Base<REQ,RESP> &&other)
        {
            if(this != &other)
            {
                this->m_handle = std::move(other.m_handle);
                this->m_task = std::move(other.m_task);
                other.m_handle = nullptr;
                other.m_task = nullptr;
            }
        }

        virtual bool await_ready()
        {
            return m_task == nullptr;
        }

        virtual void await_suspend(std::coroutine_handle<> handle)
        {
            this->m_handle = handle;
        }

    protected:
        void done()
        {
            this->m_handle.resume();
        }
    protected:
        std::coroutine_handle<> m_handle = nullptr;
        Task<REQ,RESP>::ptr m_task = nullptr;
    };
    
    template<Request REQ , Response RESP , typename RESULT>
    class Net_Awaiter: public Awaiter_Base
    {
    public:
        using ptr = std::shared_ptr<Net_Awaiter>;
        Net_Awaiter(Task<REQ,RESP>::ptr task)
            : Awaiter_Base<REQ,RESP>(task)
        {

        }

        RESULT await_resume(){
            if constexpr (!std::is_same_v<RESULT,void>)
                return std::move(m_result.value());
        }
    protected:
        std::optional<RESULT> m_result;
    };
}

#endif