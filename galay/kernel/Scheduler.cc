#include "Scheduler.h"
#include "Event.h"
#include "EventEngine.h"
#include "Coroutine.h"
#include "ExternApi.h"
#include "galay/util/Thread.h"
#include "Log.h"

namespace galay::details
{

EventScheduler::EventScheduler()
{
#if defined(USE_EPOLL)
    m_engine = std::make_shared<details::EpollEventEngine>();
#elif defined(USE_IOURING)
    m_engine = std::make_shared<IoUringEventEngine>();
#elif defined(USE_KQUEUE)
    m_engine = std::make_shared<details::KqueueEventEngine>();
#endif
    m_waiter = std::make_shared<thread::ThreadWaiters>(1);
}

bool EventScheduler::Loop(int timeout)
{
    this->m_thread = std::make_unique<std::thread>([this, timeout](){
        m_engine->Loop(timeout);
        LogTrace("[{}({}) exist successfully]", Name(), GetEngine()->GetHandle().fd);
        m_waiter->Decrease();
    });
    this->m_thread->detach();
    return true;
}

bool EventScheduler::Stop() const
{
    if(!m_engine->IsRunning()) return false;
    m_engine->Stop();
    return m_waiter->Wait(5000);
}

bool EventScheduler::IsRunning() const
{
    return m_engine->IsRunning();
}

uint32_t EventScheduler::GetErrorCode() const
{
    return m_engine->GetErrorCode();
}


CoroutineScheduler::CoroutineScheduler()
    : m_running(false)
{
    m_waiter = std::make_shared<thread::ThreadWaiters>(1);
}

void 
CoroutineScheduler::EnqueueCoroutine(coroutine::Coroutine *coroutine)
{
    m_coroutines_queue.enqueue(coroutine);
}

bool CoroutineScheduler::Loop(int timeout)
{
    this->m_thread = std::make_unique<std::thread>([this](){
        coroutine::Coroutine* co = nullptr;
        m_running = true;
        while(true)
        {
            m_coroutines_queue.wait_dequeue(co);
            if(co) {
                co->Resume();
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
    m_coroutines_queue.enqueue(nullptr);
    coroutine::GetCoroutineStore()->Clear(); 
    return m_waiter->Wait(5000);
}

bool CoroutineScheduler::IsRunning() const
{
    return m_running;
}

TimerScheduler::TimerScheduler()
{
    GHandle handle{};
    details::TimeEvent::CreateHandle(handle);
    m_timer_event = new details::TimeEvent(handle, m_engine.get());
}

std::shared_ptr<galay::Timer> TimerScheduler::AddTimer(const int64_t ms, std::function<void(std::shared_ptr<galay::Timer>)>&& callback) const
{
    return m_timer_event->AddTimer(ms, std::forward<std::function<void(std::shared_ptr<galay::Timer>)>>(callback));
}

bool TimerScheduler::Loop(const int timeout)
{
    return EventScheduler::Loop(timeout);
}

bool TimerScheduler::Stop() const
{
    return EventScheduler::Stop();
}

bool TimerScheduler::IsRunning() const
{
    return EventScheduler::IsRunning();
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