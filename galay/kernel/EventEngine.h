#ifndef GALAY_EVENTSCHEDULER_H
#define GALAY_EVENTSCHEDULER_H
#include "galay/common/Base.h"
#include <string>
#include <atomic>
#include <list>
#include <memory>
#include <functional>

namespace galay::details
{
class Event;

#define DEFAULT_MAX_EVENTS 1024


class EventEngine
{
    static std::atomic_uint32_t gEngineId;
public:

    using ptr = std::shared_ptr<EventEngine>;
    EventEngine();

    uint32_t GetEngineID() const { return m_id.load(); }
    virtual bool Loop(int timeout) = 0;
    virtual bool Stop() = 0;
    virtual int AddEvent(Event* event, void* ctx) = 0;
    virtual int ModEvent(Event* event, void* ctx) = 0;
    virtual int DelEvent(Event* event, void* ctx) = 0;
    virtual bool IsRunning() const = 0;
    virtual uint32_t GetErrorCode() const = 0;
    virtual GHandle GetHandle() = 0;
    virtual uint32_t GetMaxEventSize() = 0;
    virtual void ResetMaxEventSize(uint32_t size) = 0;
    virtual void RegisterOnceLoopCallback(std::function<void()>&& callback) = 0;
    virtual ~EventEngine() = default;
protected:
    std::atomic_uint32_t m_id;
};

#if defined(USE_EPOLL)
//default ET mode
class EpollEventEngine: public EventEngine
{
public:
    EpollEventEngine(uint32_t max_events = DEFAULT_MAX_EVENTS);
    virtual bool Loop(int timeout = -1) override;
    virtual bool Stop() override;
    virtual int AddEvent(Event* event, void* ctx) override;
    virtual int ModEvent(Event* event, void* ctx) override;
    virtual int DelEvent(Event* event, void* ctx) override;
    bool IsRunning() const override { return !m_stop; }
    virtual uint32_t GetErrorCode() const override { return m_error_code; }
    virtual GHandle GetHandle() override { return m_handle; }
    virtual uint32_t GetMaxEventSize() override { return m_event_size; }
    void RegisterOnceLoopCallback(std::function<void()>&& callback) override { m_once_loop_cbs.push_back(std::move(callback)); }

    /*
        设置步骤
            1.if size > m_event_size, then m_event_size *= 2;
            2.if size < m_event_size / 4, then m_event /= 2;
            3.m_event_size >= DEFAULT_MAX_EVENTS
    */
    virtual void ResetMaxEventSize(uint32_t size) override;
    virtual ~EpollEventEngine();
private:
    bool ConvertToEpollEvent(struct epoll_event &ev, Event *event, void* ctx);
private:
    GHandle m_handle;
    uint32_t m_error_code;
    std::atomic_bool m_stop;
    std::atomic_uint32_t m_event_size;
    std::atomic<epoll_event*> m_events;
    std::list<std::function<void()>> m_once_loop_cbs; 
};
#elif defined(USE_IOURING)
class IoUringEventEngine
{
    
};

#elif defined(USE_SELECT)

#elif defined(USE_KQUEUE)

//default ET 
class KqueueEventEngine final : public EventEngine
{
public:
    explicit KqueueEventEngine(uint32_t max_events = DEFAULT_MAX_EVENTS);
    bool Loop(int timeout = -1) override;
    bool Stop() override;
    int AddEvent(Event* event, void* ctx ) override;
    int ModEvent(Event* event, void* ctx) override;
    /*
        DelEvent will move the event from event list, please call Event.Free() to free the event
    */
    int DelEvent(Event* event, void* ctx) override;
    bool IsRunning() const override { return !m_stop; }
    uint32_t GetErrorCode() const override { return m_error_code; }
    GHandle GetHandle() override { return m_handle; }
    uint32_t GetMaxEventSize() override { return m_event_size; }
    void ResetMaxEventSize(const uint32_t size) override { m_event_size = size; }
    void RegisterOnceLoopCallback(std::function<void()>&& callback) override { m_once_loop_cbs.push_back(std::move(callback)); }
    ~KqueueEventEngine() override;
private:
    bool ConvertToKEvent(struct kevent &ev, Event *event, void* ctx);
private:
    GHandle m_handle{};
    uint32_t m_error_code;
    std::atomic_bool m_stop;
    std::atomic_uint32_t m_event_size;
    std::atomic<struct kevent*> m_events;
    std::list<std::function<void()>> m_once_loop_cbs; 
};

#endif
}

#endif