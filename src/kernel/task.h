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


namespace galay
{
    enum Task_Status
    {
        GY_TASK_STOP,
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
    class Task
    {
    public:
        using ptr = std::shared_ptr<Task>;

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

        virtual ~Task() {}

    protected:
        int m_status;
        int m_error;
    };
}

#endif