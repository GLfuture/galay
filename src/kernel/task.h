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
#include "engine.h"
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

    template <Request REQ, Response RESP>
    class Server;

    template <Request REQ, Response RESP>
    class Tcp_RW_Task;

    template <Request REQ, Response RESP>
    class Tcp_SSL_RW_Task;

    template <Request REQ, Response RESP>
    class Http_RW_Task;
    
    template <Request REQ, Response RESP>
    class Https_RW_Task;

    template <Request REQ, Response RESP>
    class Task_Base
    {
    public:
        using ptr = std::shared_ptr<Task_Base>;

        virtual std::shared_ptr<REQ> get_req() = 0;
        virtual std::shared_ptr<RESP> get_resp() = 0;
        virtual void control_task_behavior(Task_Status status) = 0;

        // return -1 error 0 success
        virtual int exec() = 0;
        virtual int get_state()
        {
            return this->m_status;
        }

        virtual int get_error()
        {
            return this->m_error;
        }

        virtual void finish(){
            this->m_is_finish = true;
        }

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