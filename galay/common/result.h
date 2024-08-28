#ifndef GALAY_RESULT_H
#define GALAY_RESULT_H

#include <memory>
#include <any>
#include "waitgroup.h"

namespace galay::result
{
    class ResultInterface
    {
    public:
        using ptr = std::shared_ptr<ResultInterface>;
        using wptr = std::weak_ptr<ResultInterface>;
        ResultInterface();
        void Done();
        bool Success();
        std::any Result();
        std::string Error();
        void AddTaskNum(uint16_t taskNum);
        coroutine::GroupAwaiter& Wait();
        void SetResult(std::any result);
        void SetErrorMsg(std::string errMsg);
        void SetSuccess(bool success);
    protected:
        bool m_success;
        std::any m_result;   
        std::string m_errMsg;
        std::shared_ptr<coroutine::WaitGroup> m_waitGroup;
    };

    //udp
}



#endif