#include "EventEngine.h"
#include "ExternApi.hpp"
#if defined(__linux__)
    #include <sys/eventfd.h>
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    
#endif
#include "Log.h"


namespace galay::details
{

std::atomic_uint32_t EventEngine::gEngineId = 0;

EventEngine::EventEngine()
    :m_id(0)
{
    gEngineId.fetch_add(1);
    m_id.store(gEngineId.load());
}

#if defined(USE_EPOLL)
EpollEventEngine::EpollEventEngine(uint32_t max_events)
{
    this->m_handle.fd = 0;
    this->m_event_size = max_events;
    this->m_events = static_cast<epoll_event*>(calloc(max_events, sizeof(epoll_event)));
    this->m_stop = true;
    this->m_handle.fd = epoll_create(1);
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
            LogTrace("[Engine Start, Engine: {}]", m_handle.fd);
            this->m_stop = false;
            nEvents = epoll_wait(m_handle.fd, m_events, m_event_size, timeout);
        }else{
            nEvents = epoll_wait(m_handle.fd, m_events, m_event_size, timeout);
        }
        if(nEvents < 0) {
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
            close(event->GetHandle().fd);
            delete static_cast<CallbackEvent*>(event);
        });
        AddEvent(event, nullptr);
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
EpollEventEngine::AddEvent(Event *event, void* ctx)
{
    LogTrace("[Add {} To Engine({}), Handle: {}, Type: {}]]", event->Name(), this->m_handle.fd, event->GetHandle().fd, ToString(event->GetEventType()));
    epoll_event ev;
    if(!ConvertToEpollEvent(ev, event, ctx))
    {
        return 0;
    }
    ev.data.ptr = event;
    int ret = epoll_ctl(m_handle.fd, EPOLL_CTL_ADD, event->GetHandle().fd, &ev);
    if( ret != 0 ){
        m_error_code = error::MakeErrorCode(error::Error_AddEventError, errno);
        return ret;
    }
    else {
        m_error_code = error::Error_NoError;
    }
    while(!event->SetEventEngine(this)){
        LogError("[SetEventEngine failed]");
    }
    return ret;
}

int 
EpollEventEngine::ModEvent(Event* event, void* ctx)
{
    LogTrace("[Mod {} In Engine({}), Handle: {}, Type: {}]]", event->Name(), this->m_handle.fd, event->GetHandle().fd, ToString(event->GetEventType()));
    epoll_event ev;
    ev.data.ptr = event;
    if( !ConvertToEpollEvent(ev, event, ctx) ) return 0;
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
EpollEventEngine::DelEvent(Event* event, void* ctx)
{
    LogTrace("[Del {} From Engine({}), Handle: {}, Type: {}]]", event->Name(), this->m_handle.fd, event->GetHandle().fd, ToString(event->GetEventType()));
    GHandle handle = event->GetHandle();
    epoll_event ev;
    ev.data.ptr = event;
    ev.events = (EPOLLIN | EPOLLOUT | EPOLLERR);
    int ret = epoll_ctl(m_handle.fd, EPOLL_CTL_DEL, handle.fd, &ev);
    if( ret != 0 ) {
        m_error_code = error::MakeErrorCode(error::Error_DelEventError, errno);
        return ret;
    }
    else {
        m_error_code = error::Error_NoError;
    }
    while(!event->SetEventEngine(nullptr)){
        LogError("[SetEventEngine failed]");
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
EpollEventEngine::ConvertToEpollEvent(epoll_event &ev, Event *event, void* ctx)
{
    int event_type = event->GetEventType();
    ev.events = 0;
    ev.events = EPOLLONESHOT;
    switch(event_type)
    {
        case kEventTypeNone:
            return false;
        case kEventTypeError:
        {
            ev.events |= EPOLLERR;
            ev.events |= EPOLLET;
        }
            break;
        case kEventTypeRead:
        {
            ev.events |= EPOLLIN;
            ev.events |= EPOLLET;
        }
            break;
        case kEventTypeWrite:
        {
            ev.events |= EPOLLOUT;
            ev.events |= EPOLLET;
        }
            break;
        case kEventTypeTimer:
            ev.events |= EPOLLIN;
            break;
    }
    return true;
}

#elif defined(USE_IOURING)


#elif defined(USE_KQUEUE)

KqueueEventEngine::KqueueEventEngine(const uint32_t max_events)
{
    m_handle.fd = kqueue();
    m_event_size = max_events;
    m_events = static_cast<struct kevent*>(calloc(max_events, sizeof(struct kevent)));
    this->m_stop = true;
    if(this->m_handle.fd < 0) {
        m_error_code = error::MakeErrorCode(error::Error_EpollCreateError, errno);
    }
}

bool KqueueEventEngine::Loop(int timeout)
{
    bool initial = true;
    timespec ts{};
    if(timeout > 0) {
        ts.tv_sec = timeout / 1000;
        ts.tv_nsec = (timeout % 1000) * 1000000;
    }
    do
    {
        int nEvents;
        if(initial) {
            initial = false;
            LogTrace("[Engine Start, Engine: {}]", m_handle.fd);
            this->m_stop = false;
            if(timeout > 0) nEvents = kevent(m_handle.fd, nullptr, 0, m_events, m_event_size, &ts);
            else nEvents = kevent(m_handle.fd, nullptr, 0, m_events, m_event_size, nullptr);
        }else{
            if(timeout > 0) nEvents = kevent(m_handle.fd, nullptr, 0, m_events, m_event_size, &ts);
            else nEvents = kevent(m_handle.fd, nullptr, 0, m_events, m_event_size, nullptr);
        }
        if(nEvents < 0) {
            continue;
        };
        for(int i = 0; i < nEvents; ++i)
        {
            Event* event = (Event*)m_events[i].udata;
            event->HandleEvent(this);
        }
    } while (!this->m_stop);
    return true;
}

bool KqueueEventEngine::Stop()
{
    if(this->m_stop.load() == false)
    {
        this->m_stop.store(true);
        int pipefd[2];
        if(pipe(pipefd) < 0) {
            return false;
        }
        int readfd = pipefd[0];
        int writefd = pipefd[1];
        GHandle handle = {readfd};
        CallbackEvent* event = new CallbackEvent(handle, EventType::kEventTypeRead, [this](Event *event, EventEngine *engine) {
                char t;
                read(event->GetHandle().fd, &t, 1);
                close(event->GetHandle().fd);
                delete static_cast<CallbackEvent*>(event);
            });
        AddEvent(event, nullptr);
        char t = 'a';
        write(writefd, &t, 1);
        close(writefd);
        return true;
    }
    return false;
}

int KqueueEventEngine::AddEvent(Event *event, void* ctx)
{
    LogTrace("[Add {} To Engine({}), Handle: {}, Type: {}]]", event->Name(), this->m_handle.fd, event->GetHandle().fd, ToString(event->GetEventType()));
    struct kevent k_event;
    k_event.flags = EV_ADD;
    if(!ConvertToKEvent(k_event, event, ctx)) {
        return 0;
    };
    int ret = kevent(m_handle.fd, &k_event, 1, nullptr, 0, nullptr);
    if(ret != 0){
        m_error_code = error::MakeErrorCode(error::Error_ModEventError, errno);
        return ret;
    }else{
        m_error_code = error::Error_NoError;
    }
    while(!event->SetEventEngine(this)){
        LogError("[SetEventEngine failed]");
    }
    return ret;
}

int KqueueEventEngine::ModEvent(Event *event, void* ctx)
{
    LogTrace("[Mod {} In Engine({}), Handle: {}, Type: {}]]", event->Name(), this->m_handle.fd, event->GetHandle().fd, ToString(event->GetEventType()));
    struct kevent k_event;
    k_event.flags = EV_ADD;
    if(!ConvertToKEvent(k_event, event, ctx)) {
        return 0;
    }
    int ret = kevent(m_handle.fd, &k_event, 1, nullptr, 0, nullptr);
    if(ret != 0){
        m_error_code = error::MakeErrorCode(error::Error_ModEventError, errno);
        return ret;
    }else{
        m_error_code = error::Error_NoError;
    }
    return ret;
}

int KqueueEventEngine::DelEvent(Event *event, void* ctx)
{
    LogTrace("[Del {} From Engine({}), Handle: {}, Type: {}]]", event->Name(), this->m_handle.fd, event->GetHandle().fd, ToString(event->GetEventType()));
    struct kevent k_event;
    k_event.flags = EV_DELETE;
    if(!ConvertToKEvent(k_event, event, ctx)) {
        return 0;
    }
    int ret = kevent(m_handle.fd, &k_event, 1, nullptr, 0, nullptr);
    if(ret != 0){
        m_error_code = error::MakeErrorCode(error::Error_DelEventError, errno);
        return ret;
    }else{
        m_error_code = error::Error_NoError;
    }
    while(!event->SetEventEngine(nullptr)){
        LogError("[SetEventEngine failed]");
    }
    return ret;
}

KqueueEventEngine::~KqueueEventEngine()
{
}

bool KqueueEventEngine::ConvertToKEvent(struct kevent &ev, Event *event, void *ctx)
{
    ev.ident = event->GetHandle().fd;
    ev.udata = event;
    ev.flags |= (EV_CLEAR | EV_ONESHOT | EV_ENABLE);
    ev.data = 0;
    ev.fflags = 0;
    switch (event->GetEventType())
    {
    case kEventTypeError:
        return false;
    case kEventTypeNone:
        return false;
    case kEventTypeRead:
        ev.filter = EVFILT_READ;
        break;
    case kEventTypeWrite:
        ev.filter = EVFILT_WRITE;
        break;
    case kEventTypeTimer:
    {
        ev.filter = EVFILT_TIMER;
        if(ctx != nullptr) {
            int64_t during_time = static_cast<Timer*>(ctx)->GetTimeout();
            ev.data = during_time;
        }
        
    }
        break;
    }
    return true;
}

#endif

}