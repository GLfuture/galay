#include "Scheduler.h"
#include "Event.h"
#include "EventEngine.h"
#include "Coroutine.h"
#include "ExternApi.h"
#include "galay/util/Thread.h"
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
#elif defined(USE_KQUEUE)
    m_engine = std::make_shared<event::KqueueEventEngine>();
#endif
    m_waiter = std::make_shared<thread::ThreadWaiters>(1);
}

bool EventScheduler::Loop(int timeout)
{
    this->m_thread = std::make_unique<std::thread>([this, timeout](){
        m_engine->Loop(timeout);
        spdlog::info("{} exit successfully!", Name());
        m_waiter->Decrease();
    });
    this->m_thread->detach();
    m_start = true;
    return true;
}

bool EventScheduler::Stop() const
{
    if(!m_start) {
        return false;
    }
    m_engine->Stop();
    GetThisThreadCoroutineStore()->Clear();
    return m_waiter->Wait(5000);
}

uint32_t EventScheduler::GetErrorCode() const
{
    return m_engine->GetErrorCode();
}

EventScheduler::~EventScheduler()
= default;

CoroutineScheduler::CoroutineScheduler()
{
    m_start = false;
    m_waiter = std::make_shared<thread::ThreadWaiters>(1);
}

void 
CoroutineScheduler::EnqueueCoroutine(coroutine::Coroutine *coroutine)
{
    m_coroutines_queue.enqueue(coroutine);
}

bool CoroutineScheduler::Loop()
{
    this->m_thread = std::make_unique<std::thread>([this](){
        coroutine::Coroutine* co = nullptr;
        while(true)
        {
            m_coroutines_queue.wait_dequeue(co);
            if(co) {
                co->Resume();
            }else{
                break;
            }
        }
        spdlog::info("{} exit successfully!", Name());
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
    GetThisThreadCoroutineStore()->Clear(); 
    return m_waiter->Wait(5000);
}



TimerScheduler::TimerScheduler()
{
    GHandle handle{};
    event::TimeEvent::CreateHandle(handle);
    m_timer_event = new event::TimeEvent(handle, m_engine.get());
}

std::shared_ptr<event::Timer> TimerScheduler::AddTimer(const int64_t ms, std::function<void(std::shared_ptr<event::Timer>)>&& callback) const
{
    return m_timer_event->AddTimer(ms, std::forward<std::function<void(std::shared_ptr<event::Timer>)>>(callback));
}

bool TimerScheduler::Loop(const int timeout)
{
    return EventScheduler::Loop(timeout);
}

bool TimerScheduler::Stop() const
{
    return EventScheduler::Stop();
}

uint32_t TimerScheduler::GetErrorCode() const
{
    return EventScheduler::GetErrorCode();
}

TimerScheduler::~TimerScheduler()
{
    delete m_timer_event;
}
}