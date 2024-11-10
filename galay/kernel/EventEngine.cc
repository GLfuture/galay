#include "EventEngine.h"
#include "Event.h"
#include "Step.h"
#include "../common/Error.h"
#include <sys/eventfd.h>
#include <cstring>
#include <spdlog/spdlog.h>

namespace galay::event
{
#if defined(USE_EPOLL)
EpollEventEngine::EpollEventEngine(uint32_t max_events)
{
    this->m_handle.fd = 0;
    this->m_event_size = max_events;
    this->m_events = (epoll_event*)malloc(sizeof(epoll_event) * max_events);
    bzero(m_events, sizeof(epoll_event) * max_events);
    this->m_stop = true;
    this->m_handle.fd = epoll_create(1);
    spdlog::debug("EpollEventEngine::EpollEventEngine [Create Engine: {}]", m_handle.fd);
    if(this->m_handle.fd < 0) {
        m_error_code = error::MakeErrorCode(error::Error_EpollCreateError, errno);
    }
}

bool 
EpollEventEngine::Loop(int timeout)
{
    bool initial = true;
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
            delete event;
        });
        AddEvent(event);
        int ret = eventfd_write(handle.fd, 1);
        if(ret < 0) {
            m_error_code = error::MakeErrorCode(error::Error_EventWriteError, errno);
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
        m_error_code = error::MakeErrorCode(error::Error_AddEventError, errno);
        return ret;
    }
    else {
        m_error_code = error::Error_NoError;
        event->BelongEngine() = this;
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
        m_error_code = error::MakeErrorCode(error::Error_ModEventError, errno);
        return ret;
    }
    else {
        m_error_code = error::Error_NoError;
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
        m_error_code = error::MakeErrorCode(error::Error_DelEventError, errno);
        return ret;
    }
    else {
        m_error_code = error::Error_NoError;
        event->BelongEngine() = nullptr;
    }
    return ret;
}

void EpollEventEngine::ResetMaxEventSize(uint32_t size)
{
    if (size == m_event_size) {
        m_event_size = m_event_size * 2;
        epoll_event* new_events = (epoll_event*)realloc(m_events, sizeof(epoll_event) * m_event_size);                                                
        if( new_events ) m_events = new_events;
    }
    if(size == m_event_size / 4 && m_event_size / 2 >= DEFAULT_MAX_EVENTS) {
        m_event_size = m_event_size / 2;
        epoll_event* new_events = (epoll_event*)realloc(m_events, sizeof(epoll_event) * m_event_size );                                                
        if( new_events ) m_events = new_events;
    }
}

EpollEventEngine::~EpollEventEngine()
{
    if(m_handle.fd > 0) close(m_handle.fd);
    free(m_events);
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