#ifndef GALAY_WAITGROUP_H
#define GALAY_WAITGROUP_H

#include "awaiter.h"
#include <atomic>

namespace galay
{
    class WaitGroup{
    public:
        WaitGroup();
        void Add(int num);
        GroupAwaiter& Wait();
        void Done();
    private:
        std::atomic_int16_t m_coNum;    
        GroupAwaiter m_awaiter;
    };
}


#endif
