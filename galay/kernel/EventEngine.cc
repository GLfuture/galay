#include "EventEngine.h"
#include "Event.h"
#include <cstring>
#include <spdlog/spdlog.h>

namespace galay::event
{
#if defined(USE_EPOLL)
EpollEventEngine::EpollEventEngine(int max_events)
{
    this->m_handle = 0;
    this->m_event_num = max_events;
    this->m_ready_events = new epoll_event[max_events];
    bzero(m_ready_events, sizeof(epoll_event) * max_events);
    this->m_stop = true;
    this->m_handle = epoll_create(1);
    spdlog::debug("EpollEventEngine::EpollEventEngine [Create Engine: {}]", m_handle);
    if(this->m_handle < 0) {
        m_error = "epoll_create: ";
        m_error += strerror(errno);
    }
}

bool 
EpollEventEngine::Loop(int timeout)
{
    bool initial = true;
    do
    {
        int nEvents;
        spdlog::debug("EpollEventEngine::Loop [Engine: {} will to wait]", this->m_handle);
        if(initial) {
            initial = false;
            this->m_stop = false;
            nEvents = epoll_wait(m_handle, m_ready_events, m_event_num, timeout);
        }else{
            nEvents = epoll_wait(m_handle, m_ready_events, m_event_num, timeout);
        }
        spdlog::debug("EpollEventEngine::Loop [Engine: {} epoll_wait return: {}]", this->m_handle, nEvents);
        if(nEvents < 0) {
            spdlog::error("EpollEventEngine::Loop [Engine: {} epoll_wait failed, error: {}]", this->m_handle, strerror(errno));
            continue;
        };
        for(int i = 0; i < nEvents; ++i)
        {
            Event* event = (Event*)m_ready_events[i].data.ptr;
            event->HandleEvent(this);
        }
    } while (!this->m_stop);
    spdlog::info("EpollEventEngine::Loop [Engine: {} exit]", this->m_handle);
    return true;
}


bool EpollEventEngine::Stop()
{
    if(this->m_stop.load() == false)
    {
        this->m_stop.store(true);
        GHandle handle = eventfd(0, EFD_NONBLOCK | EFD_SEMAPHORE | EFD_CLOEXEC);
        CallbackEvent* event = new CallbackEvent(handle, EventType::kEventTypeRead, [this](Event *event, EventEngine *engine) {
            eventfd_t val;
            int ret = eventfd_read(event->GetHandle(), &val);
            event->Free(engine);
        });
        AddEvent(event);
        int ret = eventfd_write(handle, 1);
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
    spdlog::debug("EpollEventEngine::AddEvent [Engine: {}] [Name: {}, Handle: {}, Type: {}]", this->m_handle, event->Name(), event->GetHandle(), event->GetEventType());
    epoll_event ev;
    ConvertToEpollEvent(ev, event);
    ev.data.ptr = event;
    int ret = epoll_ctl(m_handle, EPOLL_CTL_ADD, event->GetHandle(), &ev);
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
    spdlog::debug("EpollEventEngine::ModEvent [Engine: {}] [Name: {}, Handle: {}, Type: {}]", this->m_handle, event->Name(), event->GetHandle(), event->GetEventType());
    epoll_event ev;
    ev.data.ptr = event;
    ConvertToEpollEvent(ev, event);
    int ret = epoll_ctl(m_handle, EPOLL_CTL_MOD, event->GetHandle(), &ev);
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
    spdlog::debug("EpollEventEngine::DelEvent [Engine: {}] [Name: {}, Handle: {}, Type: {}]", this->m_handle, event->Name(), event->GetHandle(), event->GetEventType());
    GHandle handle = event->GetHandle();
    epoll_event ev;
    ev.data.ptr = event;
    int ret = epoll_ctl(m_handle, EPOLL_CTL_DEL, handle, &ev);
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

EpollEventEngine::~EpollEventEngine()
{
    if(m_handle > 0) close(m_handle);
    delete[] m_ready_events;
}

void 
EpollEventEngine::ConvertToEpollEvent(epoll_event &ev, Event *event)
{
    int event_type = event->GetEventType();
    ev.events = 0;
    ev.events = EPOLLET;
    if( event_type & kEventTypeRead) {
        ev.events |= EPOLLIN;
    }
    if( event_type & kEventTypeWrite) {
        ev.events |= EPOLLOUT;
    }
}

#elif defined(USE_IOURING)
    


#endif    
}