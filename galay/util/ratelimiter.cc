#include "ratelimiter.h"
#include <functional>

galay::util::GY_RateLimiter::GY_RateLimiter(uint64_t rate, uint64_t capacity,uint64_t deliveryInteralMs)
{
    this->m_rate = rate;
    this->m_semaphore = std::make_unique<CountSemaphore>(capacity,capacity);
    this->m_deliveryInteralMs = deliveryInteralMs;
    this->m_deliveryThread = nullptr;
    this->m_runing = false;
}

void
galay::util::GY_RateLimiter::Start()
{
    if(m_runing) return;
    m_runing = true;
    m_deliveryThread = std::make_unique<std::thread>(std::bind(&GY_RateLimiter::ProduceToken,this));
}

void 
galay::util::GY_RateLimiter::Stop()
{
    m_runing = false;
    if(m_deliveryThread) {
        if(m_deliveryThread->joinable()) m_deliveryThread->join();
    }
}

void 
galay::util::GY_RateLimiter::ProduceToken()
{
    auto lastTime = std::chrono::steady_clock::now();
    while(m_runing)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(m_deliveryInteralMs));
        auto curTime = std::chrono::steady_clock::now();
        auto elapseMs = std::chrono::duration<double,std::milli>(curTime - lastTime).count();
        lastTime = curTime;
        auto tokens = elapseMs * m_rate / 1000;
        m_semaphore->Put(static_cast<uint64_t>(tokens));
    }
}

bool
galay::util::GY_RateLimiter::Pass(uint64_t flow)
{
    return m_semaphore->Get(flow);
}

galay::util::GY_RateLimiter::~GY_RateLimiter()
{
    m_deliveryThread.reset(nullptr);
}