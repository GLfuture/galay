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
CoroutineScheduler::ToResumeCoroutine(Coroutine_wptr coroutine)
{
    m_coroutines_queue.enqueue({Action::kActionResume, coroutine});
}

void CoroutineScheduler::ToDestroyCoroutine(Coroutine_wptr coroutine)
{
    m_coroutines_queue.enqueue({Action::kActionDestroy, coroutine});
}

bool CoroutineScheduler::Loop(int timeout)
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
                    co.second.lock()->Resume();
                    break;
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
    coroutine::GetCoroutineStore()->Clear(); 
    m_coroutines_queue.enqueue({});
    return m_waiter->Wait(5000);
}

bool CoroutineScheduler::IsRunning() const
{
    return m_running;
}

TimerScheduler::TimerScheduler()
{
}

std::shared_ptr<galay::Timer> TimerScheduler::AddTimer(const uint64_t ms, std::function<void(std::weak_ptr<details::TimeEvent>, std::shared_ptr<galay::Timer>)>&& callback) const
{
    return m_timer_event->AddTimer(ms, std::move(callback));
}

void TimerScheduler::AddTimer(const uint64_t ms, const std::shared_ptr<Timer>& timer)
{
    m_timer_event->AddTimer(ms, timer);
}

bool TimerScheduler::Loop(const int timeout)
{
    GHandle handle{};
    details::TimeEvent::CreateHandle(handle);
    m_timer_event = new details::TimeEvent(handle, m_engine.get());
    return EventScheduler::Loop(timeout);
}

bool TimerScheduler::Stop() const
{
    delete m_timer_event; 
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
}

}