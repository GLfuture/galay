#include "Scheduler.h"
#include "Event.h"
#include "EventEngine.h"
#include "../util/Thread.h"
#include <sys/timerfd.h>

namespace galay::scheduler
{
    
std::vector<CoroutineScheduler*> g_coroutine_schedulers;
std::vector<EventScheduler*> g_netio_schedulers;

void DynamicResizeCoroutineSchedulers(int num)
{
    int now = g_coroutine_schedulers.size();
    int sub = num - now;
    if(sub > 0) {
        for(int i = 0; i < sub; ++i) {
            g_coroutine_schedulers.push_back(new CoroutineScheduler);
        }
    }else if(sub < 0) {
        for(int i = g_coroutine_schedulers.size() - 1 ; i >= -sub ; -- i)
        {
            g_coroutine_schedulers[i]->Stop();
            delete g_coroutine_schedulers[i];
            g_coroutine_schedulers.erase(std::prev(g_coroutine_schedulers.end()));
        }
    }
}

void DynamicResizeNetIOSchedulers(int num)
{
    int now = g_netio_schedulers.size();
    int sub = num - now;
    if(sub > 0) {
        for(int i = 0; i < sub; ++i) {
            g_netio_schedulers.push_back(new EventScheduler);
        }
    } else if(sub < 0) {
        for(int i = g_netio_schedulers.size() - 1 ; i >= -sub ; -- i)
        {
            g_netio_schedulers[i]->Stop();
            delete g_netio_schedulers[i];
            g_netio_schedulers.erase(std::prev(g_netio_schedulers.end()));
        }
    }
}

// void SetCoroutineSchedulerNum(int num)
// {

// }

// void SetNetIOSchedulerNum(int num)
// {

// }

int GetCoroutineSchedulerNum()
{
    return g_coroutine_schedulers.size();
}

int GetNetIOSchedulerNum()
{
    return g_netio_schedulers.size();
}

EventScheduler* GetNetIOScheduler(int index)
{
    return g_netio_schedulers[index];
}

CoroutineScheduler* GetCoroutineScheduler(int index)
{
    return g_coroutine_schedulers[index];
}

void StartCoroutineSchedulers(int timeout)
{
    for(int i = 0 ; i < g_coroutine_schedulers.size() ; ++i )
    {
        g_coroutine_schedulers[i]->Loop(timeout);
    }
}

void StartNetIOSchedulers(int timeout)
{
    for(int i = 0 ; i < g_netio_schedulers.size() ; ++i )
    {
        g_netio_schedulers[i]->Loop(timeout);
    }
}

void StopCoroutineSchedulers()
{
    for(int i = 0 ; i < g_coroutine_schedulers.size() ; ++i )
    {
        g_coroutine_schedulers[i]->Stop();
        delete g_coroutine_schedulers[i];
    }
    g_coroutine_schedulers.clear();
}

void StopNetIOSchedulers()
{
    for(int i = 0 ; i < g_netio_schedulers.size() ; ++i )
    {
        g_netio_schedulers[i]->Stop();
        delete g_netio_schedulers[i];
    }
    g_netio_schedulers.clear();
}


EventScheduler::EventScheduler()
{
#if defined(USE_EPOLL)
    m_engine = std::make_shared<event::EpollEventEngine>();
#elif defined(USE_IOURING)
    m_engine = std::make_shared<IoUringEventEngine>();
#endif
    m_waiter = std::make_shared<thread::ThreadWaiters>(1);
}

bool EventScheduler::Loop(int timeout)
{
    GHandle handle{
        .fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC)
    };
    m_time_event = new event::TimeEvent(handle);
    this->m_thread = std::make_unique<std::thread>([this, timeout](){
        m_engine->Loop(timeout);
        m_time_event->Free(m_engine.get());
        m_waiter->Decrease();
    });
    this->m_thread->detach();
    m_engine->AddEvent(m_time_event);
    return true;
}

bool EventScheduler::Stop()
{
    m_engine->Stop();
    return m_waiter->Wait(5000);
}

std::string EventScheduler::GetLastError()
{
    return m_engine->GetLastError();
}

int 
EventScheduler::AddEvent(event::Event* event)
{
    return m_engine->AddEvent(event);
}

int EventScheduler::ModEvent(event::Event* event)
{
    return m_engine->ModEvent(event);
}

int 
EventScheduler::DelEvent(event::Event* event)
{
    return m_engine->DelEvent(event);
}

event::TimeEvent* EventScheduler::GetTimeEvent()
{
    return m_time_event;    
}

EventScheduler::~EventScheduler()
{
}

CoroutineScheduler::CoroutineScheduler()
{
#if defined(USE_EPOLL)
    m_engine = std::make_shared<event::EpollEventEngine>();
#elif defined(USE_IOURING)
    m_engine = std::make_shared<event::IoUringEventEngine>();
#endif
    m_waiter = std::make_shared<thread::ThreadWaiters>(1);
}

void 
CoroutineScheduler::ResumeCoroutine(coroutine::Coroutine *coroutine)
{
    m_coroutine_event->ResumeCoroutine(coroutine);
}

bool CoroutineScheduler::Loop(int timeout)
{
    this->m_thread = std::make_unique<std::thread>([this, timeout](){
        m_engine->Loop(timeout);
        m_coroutine_event->Free(m_engine.get());
        m_waiter->Decrease();
    });
    this->m_thread->detach();
    GHandle handle{
        .fd = eventfd(0, EFD_NONBLOCK | EFD_SEMAPHORE | EFD_CLOEXEC)
    };
    this->m_coroutine_event = new event::CoroutineEvent(handle, m_engine.get(), event::EventType::kEventTypeRead);
    m_engine->AddEvent(this->m_coroutine_event);
    return true;
}

bool CoroutineScheduler::Stop()
{
    m_engine->Stop();
    return m_waiter->Wait(5000);
}

CoroutineScheduler::~CoroutineScheduler()
{

}


}