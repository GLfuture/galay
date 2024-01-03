#ifndef GALAY_TASK_AWAIT_H
#define GALAY_TASK_AWAIT_H

#include "task.h"


namespace galay
{
    template<Request REQ ,Response RESP , typename RESULT>
    class Co_Task_Base:public Task_Base<REQ,RESP>,public Coroutine<RESULT>
    {
    public:
        using ptr = std::shared_ptr<Co_Task_Base>;

        virtual void set_co_handle(std::coroutine_handle<> handle)
        {
            this->m_handle = handle;
        }

        int exec() override
        {
            this->m_handle.resume();
        }
    protected:
        
    };


}


#endif