#ifndef __GALAY_EVENTSCHEDULER_H__
#define __GALAY_EVENTSCHEDULER_H__
#include "Base.h"
#include "../util/ThreadSecurity.hpp"
#include <ctype.h>
#include <string>
#include <memory>
#include <atomic>
#include <vector>

namespace galay::event
{
    #define MAX_EVENTS 1024

class EventEngine;
class Event;

class EventEngine
{
public:
    using ptr = std::shared_ptr<EventEngine>;
    virtual bool Loop(int timeout = -1) = 0;
    virtual bool Stop() = 0;
    virtual int AddEvent(Event* event) = 0;
    virtual int ModEvent(Event* event) = 0;
    /*
        DelEvent will move the event from event list, please call Event.Free() to free the event
    */
    virtual int DelEvent(Event* event) = 0;
    virtual std::string GetLastError() const = 0;
    virtual GHandle GetHandle() = 0;
};

#if defined(USE_EPOLL)
//default ET mode
class EpollEventEngine: public EventEngine
{
public:
    EpollEventEngine(int max_events = MAX_EVENTS);
    virtual bool Loop(int timeout = -1) override;
    virtual bool Stop() override;
    virtual int AddEvent(Event* event) override;
    virtual int ModEvent(Event* event) override;
    virtual int DelEvent(Event* event) override;
    inline virtual std::string GetLastError() const override { return m_error; }
    inline virtual GHandle GetHandle() override { return m_handle; }
    virtual ~EpollEventEngine();
private:
    void ConvertToEpollEvent(struct epoll_event &ev, Event *event);
private:
    GHandle m_handle;
    int m_event_num;
    std::string m_error;
    epoll_event *m_ready_events;
    std::atomic_bool m_stop;
    thread::security::SecurityList<Event*> m_event_list;
};
#elif defined(USE_IOURING)
class IoUringEventEngine
{
    
};

#elif defined(USE_SELECT)

#elif defined(__APPLE__)
#endif
}

#endif