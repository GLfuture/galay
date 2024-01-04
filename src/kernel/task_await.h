#ifndef GALAY_TASK_AWAIT_H
#define GALAY_TASK_AWAIT_H

#include "task.h"


namespace galay
{
    template<Request REQ ,Response RESP , typename RESULT>
    class Co_Task_Base : public Task_Base<REQ,RESP>
    {
    public:
        using ptr = std::shared_ptr<Co_Task_Base>;

        std::shared_ptr<REQ> get_req() override
        {
            return nullptr;
        }
        std::shared_ptr<RESP> get_resp() override
        {  
            return nullptr;

        }
        void control_task_behavior(Task_Status status) override
        {

        }

        virtual void set_co_handle(std::coroutine_handle<> handle)
        {
            this->m_handle = handle;
        }

        virtual int exec() = 0 ;

        RESULT& result()
        {
            return this->m_result;
        }

        virtual ~Co_Task_Base()
        {
            if(m_handle){
                m_handle = nullptr;
            }
        }

    protected:
        RESULT m_result;
        std::coroutine_handle<> m_handle;
    };

    template<Request REQ ,Response RESP , typename RESULT = int>
    class Co_Tcp_Client_Connect_Task: public Co_Task_Base<REQ,RESP,RESULT>
    {
    public:
        using ptr = std::shared_ptr<Co_Tcp_Client_Connect_Task<REQ,RESP,RESULT>>;

        Co_Tcp_Client_Connect_Task(int fd)
        {
            this->m_fd = fd;
        }

        int exec()
        {
            int status = 0;
            socklen_t slen = 0;
            if(getsockopt(this->m_fd,SOL_SOCKET,SO_ERROR,(void*)&status,&slen) < 0){
                this->m_result = -1;
            }
            if(status != 0){
                this->m_result = -1;
            }else{
                this->m_result = 0;
            }
            if(!this->m_handle.done()) this->m_handle.resume();
            return 0;
        }
    protected:
        int m_fd;
    };


}


#endif