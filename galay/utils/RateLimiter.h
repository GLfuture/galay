#ifndef __GALAY_RATELIMITER_H__
#define __GALAY_RATELIMITER_H__

#include <cstdint>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace galay::utils
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

    class RateLimiter{
    public:
        //rate --> bytes/s , capacity --> box's capacity , deliveryIntervalMs --> during time to create token (ms)
        //最大可以应对(rate + capity) 的突发流量

        //deliveryInteralMs越大，每一次delivery的token越多，但是token更新频率低，性能提升，及时性下降
        //;反之，每一次delivery的token越少，但是token更新频率高，性能降低，及时性提高
        RateLimiter(uint64_t rate, uint64_t capacity,uint64_t deliveryInteralMs);
        void Start();
        void Stop();
        bool Pass(uint64_t flow);
        ~RateLimiter();
    private:
        void ProduceToken();
    private:
        uint64_t m_deliveryInteralMs;
        uint64_t m_rate;
        bool m_runing;
        CountSemaphore::uptr m_semaphore;
        std::unique_ptr<std::thread> m_deliveryThread;
    };
}


#endif