#ifndef GALAY_COUNTSEMAPHORE_H
#define GALAY_COUNTSEMAPHORE_H

#include <cstdint>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace galay
{
    namespace util
    {
        class CountSemaphore
        {
        public:
            using ptr = std::shared_ptr<CountSemaphore>;
            using uptr = std::unique_ptr<CountSemaphore>;
            CountSemaphore(uint64_t initcount, uint64_t capacity);
            bool Get(uint64_t count);
            void Put(uint64_t count);
        private:
            std::mutex m_mtx;
            uint64_t m_capacity;
            uint64_t m_nowcount;
            std::condition_variable m_cond;
        };
    }
}

#endif