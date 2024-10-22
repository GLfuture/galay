#ifndef __GALAY_SCHEDULER_H__
#define __GALAY_SCHEDULER_H__

#include <string>
#include <memory>
#include <thread>
#include <functional>

namespace galay::event {
    class Event;
    class TimeEvent;
    class EventEngine;
    class CoroutineEvent;   
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
    std::string GetLastError();
    int AddEvent(event::Event *event);
    int ModEvent(event::Event *event);
    int DelEvent(event::Event *event);
    event::TimeEvent* GetTimeEvent();
    inline event::EventEngine* GetEngine() { return m_engine.get(); }
    virtual ~EventScheduler();
private:
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
    void ResumeCoroutine(coroutine::Coroutine* coroutine);
    bool Loop(int timeout = -1);
    bool Stop();
    inline event::EventEngine* GetEngine() { return m_engine.get(); }
    virtual ~CoroutineScheduler();
private:
    std::unique_ptr<std::thread> m_thread;
    std::shared_ptr<event::EventEngine> m_engine;
    event::CoroutineEvent* m_coroutine_event;
    std::shared_ptr<thread::ThreadWaiters> m_waiter;
};


extern void DynamicResizeCoroutineSchedulers(int num);
extern void DynamicResizeNetIOSchedulers(int num);

// extern void SetCoroutineSchedulerNum(int num);
// extern void SetNetIOSchedulerNum(int num);
extern int GetCoroutineSchedulerNum();
extern int GetNetIOSchedulerNum();

extern EventScheduler* GetNetIOScheduler(int index);
extern CoroutineScheduler* GetCoroutineScheduler(int index);

extern void StartCoroutineSchedulers(int timeout = -1);
extern void StartNetIOSchedulers(int timeout = -1);
extern void StopCoroutineSchedulers();
extern void StopNetIOSchedulers();

}

#endif