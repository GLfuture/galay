#ifndef GALAY_BASE_H
#define GALAY_BASE_H

#include <memory>
#include <any>
#include "basic_concepts.h"
#include "../protocol/basic_protocol.h"
#include "../config/config.h"
#include "error.h"
#include "timer.h"

namespace galay
{
    enum Task_Status
    {
        GY_TASK_CONNECT,
        GY_TASK_SSL_CONNECT,
        GY_TASK_READ,
        GY_TASK_WRITE,
    };

    enum Event_type{
        GY_EVENT_READ = 0x1,
        GY_EVENT_WRITE = 0x2,
        GY_EVENT_ERROR = 0x4,
        GY_EVENT_EPOLLET = 0x8, //epoll et模式
    };

    class Scheduler_Base;


    class Task_Base
    {
    public:
        using wptr = std::weak_ptr<Task_Base>;
        using ptr = std::shared_ptr<Task_Base>;

        // return -1 error 0 success
        virtual int Exec() = 0;

        virtual std::shared_ptr<Scheduler_Base> GetScheduler()
        {
            return nullptr;
        }

        virtual Tcp_Request_Base::ptr GetReq()
        {
            return nullptr;
        }
        virtual Tcp_Response_Base::ptr GetResp()
        {
            return nullptr;
        }

        virtual void CntlTaskBehavior(Task_Status status) {}

        virtual int GetStatus() { return this->m_status; }

        virtual void Finish() { this->m_is_finish = true; }

        virtual std::any& GetContext() { return this->m_ctx; }

        virtual void Destory() { this->m_destroy = true; }

        virtual bool IsDestory() { return this->m_destroy; }

        virtual int GetError() {   return this->m_error;   }

        virtual ~Task_Base() {
            
        }

    protected:
        int m_error = Error::NoError::GY_SUCCESS;
        int m_status;
        bool m_is_finish = false;
        bool m_destroy = false;
        std::any m_ctx;
    };


    class Scheduler_Base
    {
    public:
        using ptr = std::shared_ptr<Scheduler_Base>;
        using wptr = std::weak_ptr<Scheduler_Base>;
        virtual std::shared_ptr<Timer_Manager> GetTimerManager() = 0;
        virtual void AddTask(std::pair<int,std::shared_ptr<Task_Base>>&& pair) = 0;
        virtual void DelTask(int fd) = 0;
        virtual int AddEvent(int fd, int event_type) = 0;
        virtual int DelEvent(int fd, int event_type) = 0;
        virtual int ModEvent(int fd, int from , int to) = 0;
        virtual void CloseConn(int fd) = 0;
        //is stoped?
        virtual bool IsStop() = 0;
        virtual int Start() = 0;
        virtual void Stop() = 0;
        virtual ~Scheduler_Base(){}
    };


}


#endif