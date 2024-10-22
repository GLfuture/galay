#ifndef __GALAY_EVENTSCHEDULER_H__
#define __GALAY_EVENTSCHEDULER_H__
#include "../common/Base.h"
#include <ctype.h>
#include <string>
#include <memory>
#include <atomic>
#include <vector>

namespace galay::event
{
    #define DEFAULT_MAX_EVENTS 1024

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
    virtual uint32_t GetMaxEventSize() = 0;
    virtual void ResetMaxEventSize(uint32_t size) = 0; 
};

#if defined(USE_EPOLL)
//default ET mode
class EpollEventEngine: public EventEngine
{
public:
    EpollEventEngine(uint32_t max_events = DEFAULT_MAX_EVENTS);
    virtual bool Loop(int timeout = -1) override;
    virtual bool Stop() override;
    virtual int AddEvent(Event* event) override;
    virtual int ModEvent(Event* event) override;
    virtual int DelEvent(Event* event) override;
    inline virtual std::string GetLastError() const override { return m_error; }
    inline virtual GHandle GetHandle() override { return m_handle; }
    inline virtual uint32_t GetMaxEventSize() override { return m_event_size; }
    virtual void ResetMaxEventSize(uint32_t size) override;
    virtual ~EpollEventEngine();
private:
    bool ConvertToEpollEvent(struct epoll_event &ev, Event *event);
private:
    GHandle m_handle;
    uint32_t m_event_size;
    std::string m_error;
    epoll_event *m_events;
    std::atomic_bool m_stop;
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