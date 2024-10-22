#ifndef __GALAY_STEP_H__
#define __GALAY_STEP_H__

#include <functional>
#include "../common/Base.h"

namespace galay {
    

/*
    每一次创建socket成功后会调用
*/
class AfterCreateTcpSocketStep {
public:
    static void Execute(GHandle handle);
    static void Setup(const std::function<void(GHandle)> &callback);
private:
    static std::function<void(GHandle)> m_callback;
};    
    
}

#endif