#include "Scheduler.h"
#include "EventEngine.h"
#include "Coroutine.hpp"
#include "ExternApi.hpp"
#include "Time.hpp"
#include "galay/utils/Thread.h"
#include "Log.h"

namespace galay::details
{

EventScheduler::EventScheduler()
    : m_latch(1)
{
#if defined(USE_EPOLL)
    m_engine = std::make_shared<details::EpollEventEngine>();
#elif defined(USE_IOURING)
    m_engine = std::make_shared<IoUringEventEngine>();
#elif defined(USE_KQUEUE)
    m_engine = std::make_shared<details::KqueueEventEngine>();
#endif
}

void EventScheduler::InitTimeEvent(TimerManagerType type)
{
    GHandle handle{};
    TimeEvent::CreateHandle(handle);
    m_timer_event = std::make_shared<details::TimeEvent>(handle, m_engine.get(), type);
}

bool EventScheduler::Loop()
{
    this->m_thread = std::make_unique<std::thread>([this](){
        int timeout = -1;
        if( m_timer_event ) timeout = m_timer_event->OnceLoopTimeout();
        m_engine->Loop(timeout);
        LogTrace("[{}({}) exist successfully]", Name(), GetEngine()->GetHandle().fd);
        m_latch.count_down();
    });
    this->m_thread->detach();
    return true;
}

bool EventScheduler::Stop()
{
    m_timer_event.reset();
    if(!m_engine->IsRunning()) return false;
    m_engine->Stop();
    int count = 1;
    do {
        if(count ++ >= 5) {
            if(m_latch.try_wait()) return true;
        } else break;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    } while(true);
    return m_latch.try_wait();
}

bool EventScheduler::IsRunning() const
{
    return m_engine->IsRunning();
}

uint32_t EventScheduler::GetErrorCode() const
{
    return m_engine->GetErrorCode();
}


void EventScheduler::AddTimer(timer_ptr timer, int64_t ms)
{
    m_timer_event->AddTimer(timer, ms);
}

CoroutineScheduler::CoroutineScheduler()
    : m_running(false)
{
    m_waiter = std::make_shared<thread::ThreadWaiters>(1);
}

void 
CoroutineScheduler::ToResumeCoroutine(Coroutine_wptr coroutine)
{
    m_coroutines_queue.enqueue({Action::kActionResume, coroutine});
}

void CoroutineScheduler::ToDestroyCoroutine(Coroutine_wptr coroutine)
{
    m_coroutines_queue.enqueue({Action::kActionDestroy, coroutine});
}

bool CoroutineScheduler::Loop()
{
    this->m_thread = std::make_unique<std::thread>([this](){
        std::pair<Action, Coroutine_wptr> co;
        m_running = true;
        while(true)
        {
            m_coroutines_queue.wait_dequeue(co);
            if(!co.second.expired()) {
                switch (co.first)
                {
                case Action::kActionResume:
                {
                    if(co.second.lock()->IsSuspend()) {
                        co.second.lock()->Resume();
                    }
                    break;
                }
                case Action::kActionDestroy:
                {
                    co.second.lock()->Destroy();
                    break;
                }
                default:
                    break;
                }
            }else{
                break;
            }
        }
        LogTrace("[{} exist successfully]", Name());
        m_waiter->Decrease();
    });
    this->m_thread->detach();
    return true;
}

bool CoroutineScheduler::Stop()
{
    if(!m_running) {
        return false;
    }
    //GetCoroutineStore()->Clear(); 
    m_coroutines_queue.enqueue({});
    return m_waiter->Wait(5000);
}

bool CoroutineScheduler::IsRunning() const
{
    return m_running;
}

}