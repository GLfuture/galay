#ifndef GALAY_WAITGROUP_H
#define GALAY_WAITGROUP_H

#include "awaiter.h"
#include <atomic>

namespace galay
{
    namespace kernel
    {
        class WaitGroup
        {
        public:
            using ptr = std::shared_ptr<WaitGroup>;
            using wptr = std::weak_ptr<WaitGroup>;
            using uptr = std::unique_ptr<WaitGroup>;
            WaitGroup();
            void Add(int num);
            GroupAwaiter &Wait();
            void Done();

        private:
            std::atomic_int16_t m_coNum;
            GroupAwaiter m_awaiter;
        };
    }
}

#endif
