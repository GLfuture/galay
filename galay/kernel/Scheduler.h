#ifndef __GALAY_SCHEDULER_H__
#define __GALAY_SCHEDULER_H__

#include <string>
#include <memory>
#include <thread>
#include <mutex>
#include <queue>
#include <shared_mutex>
#include <functional>
#include "concurrentqueue/moodycamel/blockingconcurrentqueue.h"
#include "../common/Base.h"

namespace galay::event {
    class Event;
    class CallbackEvent;
    class TimeEvent;
    class EventEngine; 
};

namespace galay::coroutine {
    class Coroutine;
};

namespace galay::thread {
    class ThreadWaiters;
}

namespace galay::scheduler {

class EventScheduler
{
public:
    using ptr = std::shared_ptr<EventScheduler>;
    EventScheduler();
    bool Loop(int timeout = -1);
    bool Stop();
    uint32_t GetErrorCode();
    int AddEvent(event::Event *event);
    int ModEvent(event::Event *event);
    int DelEvent(event::Event *event);
    event::TimeEvent* GetTimeEvent();
    inline event::EventEngine* GetEngine() { return m_engine.get(); }
    virtual ~EventScheduler();
private:
    std::atomic_bool m_start;
    event::TimeEvent* m_time_event;
    std::unique_ptr<std::thread> m_thread;
    std::shared_ptr<event::EventEngine> m_engine;
    std::shared_ptr<thread::ThreadWaiters> m_waiter;
};

class CoroutineScheduler
{
public:
    using ptr = std::shared_ptr<CoroutineScheduler>;
    CoroutineScheduler();
    void EnqueueCoroutine(coroutine::Coroutine* coroutine);
    bool Loop(int timeout = -1);
    bool Stop();
private:
    std::atomic_bool m_start;
    std::unique_ptr<std::thread> m_thread;
    std::shared_ptr<thread::ThreadWaiters> m_waiter;
    moodycamel::BlockingConcurrentQueue<coroutine::Coroutine*> m_coroutines_queue;
};

}

#endif