#ifndef GALAY_TASK_H
#define GALAY_TASK_H

#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <chrono>
#include "iofunction.h"
#include "basic_concepts.h"
#include "error.h"
#include "scheduler.h"
#include "coroutine.h"
#include "../protocol/tcp.h"
#include "../protocol/http.h"


namespace galay
{
    enum Task_Status
    {
        GY_TASK_DISCONNECT,
        GY_TASK_CONNECT,
        GY_TASK_READ,
        GY_TASK_WRITE,
    };

    class IO_Scheduler;


    class Task_Base
    {
    public:
        using wptr = std::weak_ptr<Task_Base>;
        using ptr = std::shared_ptr<Task_Base>;

        // return -1 error 0 success
        virtual int exec() = 0;

        virtual std::shared_ptr<IO_Scheduler> get_scheduler() {
            return nullptr;
        }

        virtual Request_Base::ptr get_req(){
            return nullptr;
        }
        virtual Response_Base::ptr get_resp(){
            return nullptr;
        }

        virtual void control_task_behavior(Task_Status status){}

        virtual int get_state() {return this->m_status;}

        virtual int get_error() {return this->m_error;}

        virtual void finish(){ this->m_is_finish = true;}

        virtual ~Task_Base() {}

    protected:
        int m_status;
        int m_error;
        bool m_is_finish = false;
    };

    template<typename RESULT = void>
    class Task: public Coroutine<RESULT>
    {
    public:
        using promise_type = Promise<RESULT>;
        Task()
        {
        }

        Task(std::coroutine_handle<promise_type> co_handle) noexcept
            : Coroutine<RESULT>(co_handle)
        {
        }

        Task(Task<RESULT> &&other) noexcept
            : Coroutine<RESULT>(other)
        {
        }

        Task<RESULT> &operator=(const Task<RESULT> &other) = delete;

        Task<RESULT> &operator=(Task<RESULT> &&other)
        {
            this->m_handle = other.m_handle;
            other.m_handle = nullptr;
            return *this;
        }
    };
}

#endif