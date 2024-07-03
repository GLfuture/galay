#ifndef GALAY_WAITGROUP_H
#define GALAY_WAITGROUP_H

#include "awaiter.h"
#include <atomic>

namespace galay
{
    namespace common
    {
        class GY_NetCoroutinePool;
    }
    namespace kernel
    {
        class WaitGroup
        {
        public:
            using ptr = std::shared_ptr<WaitGroup>;
            using wptr = std::weak_ptr<WaitGroup>;
            using uptr = std::unique_ptr<WaitGroup>;
            WaitGroup(std::weak_ptr<common::GY_NetCoroutinePool> coPool);
            void Add(int num);
            GroupAwaiter &Wait();
            void Done();

        private:
            std::atomic_int16_t m_coNum;
            GroupAwaiter m_awaiter;
            std::weak_ptr<common::GY_NetCoroutinePool> m_coPool;
        };
    }
}

#endif
