#include "EventEngine.h"
#include "Event.h"
#include "Step.h"
#include <cstring>
#include <spdlog/spdlog.h>

namespace galay::event
{
#if defined(USE_EPOLL)
EpollEventEngine::EpollEventEngine(uint32_t max_events)
{
    this->m_handle.fd = 0;
    this->m_event_size = max_events;
    this->m_events = new epoll_event[max_events];
    bzero(m_events, sizeof(epoll_event) * max_events);
    this->m_stop = true;
    this->m_handle.fd = epoll_create(1);
    spdlog::debug("EpollEventEngine::EpollEventEngine [Create Engine: {}]", m_handle.fd);
    if(this->m_handle.fd < 0) {
        m_error = "epoll_create: ";
        m_error += strerror(errno);
    }
}

bool 
EpollEventEngine::Loop(int timeout)
{
    bool initial = true;
    /*
        设置步骤
            1.if GHandle > m_event_size, then m_event_size *= 2;
            2.if GHandle < m_event_size / 4, then m_event /= 2;
            3.m_event_size >= DEFAULT_MAX_EVENTS
    */
    AfterCreateTcpSocketStep::Setup([this](GHandle handle){
        if (handle.fd >= m_event_size) {
            ResetMaxEventSize(2 * m_event_size);
        }else {
            int capacity = m_event_size / 4 ;
            if( handle.fd <= capacity ) {
                if( m_event_size / 2 >= DEFAULT_MAX_EVENTS ) {
                    ResetMaxEventSize(m_event_size / 2);
                }
            }
        }
    });
    do
    {
        int nEvents;
        if(initial) {
            initial = false;
            this->m_stop = false;
            nEvents = epoll_wait(m_handle.fd, m_events, m_event_size, timeout);
        }else{
            nEvents = epoll_wait(m_handle.fd, m_events, m_event_size, timeout);
        }
        if(nEvents < 0) {
            spdlog::error("EpollEventEngine::Loop [Engine: {} epoll_wait failed, error: {}]", this->m_handle.fd, strerror(errno));
            continue;
        };
        for(int i = 0; i < nEvents; ++i)
        {
            Event* event = (Event*)m_events[i].data.ptr;
            event->HandleEvent(this);
        }
    } while (!this->m_stop);
    spdlog::info("EpollEventEngine::Loop [Engine: {} exit]", this->m_handle.fd);
    return true;
}


bool EpollEventEngine::Stop()
{
    if(this->m_stop.load() == false)
    {
        this->m_stop.store(true);
        GHandle handle{
            .fd = eventfd(0, EFD_NONBLOCK | EFD_SEMAPHORE | EFD_CLOEXEC)
        };
        CallbackEvent* event = new CallbackEvent(handle, EventType::kEventTypeRead, [this](Event *event, EventEngine *engine) {
            eventfd_t val;
            int ret = eventfd_read(event->GetHandle().fd, &val);
            event->Free(engine);
        });
        AddEvent(event);
        int ret = eventfd_write(handle.fd, 1);
        if(ret < 0) {
            spdlog::error("EventScheduler::Stop eventfd_write failed, error: {}", strerror(errno));
            this->m_error = strerror(errno);
            return false;
        }
        return true;
    }
    return false;
}

int 
EpollEventEngine::AddEvent(Event *event)
{
    spdlog::debug("EpollEventEngine::AddEvent [Engine: {}] [Name: {}, Handle: {}, Type: {}]", this->m_handle.fd, event->Name(), event->GetHandle().fd, event->GetEventType());
    epoll_event ev;
    if(!ConvertToEpollEvent(ev, event))
    {
        return -1;
    }
    ev.data.ptr = event;
    int ret = epoll_ctl(m_handle.fd, EPOLL_CTL_ADD, event->GetHandle().fd, &ev);
    if( ret != 0 ){
        m_error = strerror(errno);
        return ret;
    }
    else {
        if (!m_error.empty()){
            m_error.clear();
        }
        event->SetEventInEngine(true);
    }
    return ret;
}

int 
EpollEventEngine::ModEvent(Event* event)
{
    spdlog::debug("EpollEventEngine::ModEvent [Engine: {}] [Name: {}, Handle: {}, Type: {}]", this->m_handle.fd, event->Name(), event->GetHandle().fd, event->GetEventType());
    epoll_event ev;
    ev.data.ptr = event;
    if( !ConvertToEpollEvent(ev, event) )
    {
        return -1;
    }
    int ret = epoll_ctl(m_handle.fd, EPOLL_CTL_MOD, event->GetHandle().fd, &ev);
    if( ret != 0 ) {
        m_error = strerror(errno);
        return ret;
    }
    else {
        if (!m_error.empty()){
            m_error.clear();
        }
    }
    return ret;
}

int 
EpollEventEngine::DelEvent(Event* event)
{
    spdlog::debug("EpollEventEngine::DelEvent [Engine: {}] [Name: {}, Handle: {}, Type: {}]", this->m_handle.fd, event->Name(), event->GetHandle().fd, event->GetEventType());
    GHandle handle = event->GetHandle();
    epoll_event ev;
    ev.data.ptr = event;
    int ret = epoll_ctl(m_handle.fd, EPOLL_CTL_DEL, handle.fd, &ev);
    if( ret != 0 ) {
        m_error = strerror(errno);
        return ret;
    }
    else {
        if (!m_error.empty()){
            m_error.clear();
        }
        event->SetEventInEngine(false);
    }
    return ret;
}

void EpollEventEngine::ResetMaxEventSize(uint32_t size)
{
    m_event_size = size;
    m_events = (epoll_event*)realloc(m_events, sizeof(epoll_event) * size);
}

EpollEventEngine::~EpollEventEngine()
{
    if(m_handle.fd > 0) close(m_handle.fd);
    delete[] m_events;
}

bool 
EpollEventEngine::ConvertToEpollEvent(epoll_event &ev, Event *event)
{
    int event_type = event->GetEventType();
    if( event_type == kEventTypeNone) return false;
    ev.events = 0;
    ev.events = EPOLLET | EPOLLONESHOT;
    if( event_type & kEventTypeRead) {
        ev.events |= EPOLLIN;
    }
    if( event_type & kEventTypeWrite) {
        ev.events |= EPOLLOUT;
    }
    return true;
}

#elif defined(USE_IOURING)
    


#endif    
}