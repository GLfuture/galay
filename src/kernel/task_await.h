#ifndef GALAY_TASK_AWAIT_H
#define GALAY_TASK_AWAIT_H

#include "task.h"


namespace galay
{
    template<Request REQ ,Response RESP , typename RESULT>
    class Co_Task:public Task_Base<REQ,RESP>,public Coroutine<RESULT>
    {
    public:
        int exec() override
        {
            this->m_handle.resume();
        }
    protected:
        
    };


}


#endif