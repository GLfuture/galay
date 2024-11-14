#include "Scheduler.h"
#include "Event.h"
#include "EventEngine.h"
#include "Coroutine.h"
#include "../util/Thread.h"
#include <spdlog/spdlog.h>

namespace galay::scheduler
{

EventScheduler::EventScheduler()
{
    m_start = false;
#if defined(USE_EPOLL)
    m_engine = std::make_shared<event::EpollEventEngine>();
#elif defined(USE_IOURING)
    m_engine = std::make_shared<IoUringEventEngine>();
#endif
    m_waiter = std::make_shared<thread::ThreadWaiters>(1);
}

bool EventScheduler::Loop(int timeout)
{
    this->m_thread = std::make_unique<std::thread>([this, timeout](){
        m_engine->Loop(timeout);
        spdlog::info("EventScheduler::Loop exit");
        m_waiter->Decrease();
    });
    this->m_thread->detach();
    m_start = true;
    return true;
}

bool EventScheduler::Stop()
{
    if(!m_start) {
        return false;
    }
    m_engine->Stop();
    return m_waiter->Wait(5000);
}

uint32_t EventScheduler::GetErrorCode()
{
    return m_engine->GetErrorCode();
}

EventScheduler::~EventScheduler()
{
}

CoroutineScheduler::CoroutineScheduler()
{
    m_start = false;
    m_waiter = std::make_shared<thread::ThreadWaiters>(1);
}

void 
CoroutineScheduler::EnqueueCoroutine(coroutine::Coroutine *coroutine)
{
    m_coroutines_queue.enqueue(std::move(coroutine));
}

bool CoroutineScheduler::Loop(int timeout)
{
    this->m_thread = std::make_unique<std::thread>([this, timeout](){
        coroutine::Coroutine* co = nullptr;
        while(1)
        {
            m_coroutines_queue.wait_dequeue(co);
            if(co) {
                co->Resume();
            }else{
                break;
            }
        }
        spdlog::info("CoroutineScheduler::Loop exit");
        m_waiter->Decrease();
    });
    this->m_thread->detach();
    m_start = true;
    return true;
}

bool CoroutineScheduler::Stop()
{
    if(!m_start) {
        return false;
    }
    m_coroutines_queue.enqueue(nullptr);
    return m_waiter->Wait(5000);
}

}