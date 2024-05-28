#include "scheduler.h"
#include "objector.h"
#include <sys/select.h>
#include <spdlog/spdlog.h>

galay::GY_SelectScheduler::GY_SelectScheduler(GY_TcpServerBuilderBase::ptr builder)
{
    FD_ZERO(&m_rfds);
    FD_ZERO(&m_wfds);
    FD_ZERO(&m_efds);
    this->m_stop = false;
    this->m_builder = builder;
    this->m_userFunc = builder->GetUserFunction();
    this->m_illegalFunc = builder->GetIllegalFunction();
    this->m_timerfd = 0;
}

galay::GY_TcpServerBuilderBase::wptr 
galay::GY_SelectScheduler::GetTcpServerBuilder()
{
    return this->m_builder;
}

/// @brief 
/// @param controller 
/// @return coroutine
galay::GY_TcpCoroutine<galay::CoroutineStatus> 
galay::GY_SelectScheduler::UserFunction(GY_Controller::ptr controller)
{
    if(!this->m_userFunc) return {};
    return this->m_userFunc(controller);
}


galay::GY_TcpCoroutine<galay::CoroutineStatus> 
galay::GY_SelectScheduler::IllegalFunction(std::string& rbuffer,std::string& wbuffer)
{
    if(!this->m_illegalFunc) return {};
    return this->m_illegalFunc(rbuffer,wbuffer);
}

void 
galay::GY_SelectScheduler::RegisterObjector(int fd, ::std::shared_ptr<GY_Objector> objector)
{
    m_objectors[fd] = objector;
}

void 
galay::GY_SelectScheduler::RegiserTimerManager(int fd, ::std::shared_ptr<GY_TimerManager> timerManager)
{
    this->m_timerfd = fd;
    RegisterObjector(fd,timerManager);
    AddEvent(m_timerfd, EventType::GY_EVENT_ERROR | EventType::GY_EVENT_READ);
}

void 
galay::GY_SelectScheduler::DelObjector(int fd)
{
    m_objectors.erase(fd);
    close(fd);
}

int 
galay::GY_SelectScheduler::DelEvent(int fd, int event_type)
{
    if(fd >= __FD_SETSIZE) return -1;
    if ((event_type & GY_EVENT_READ) != 0)
    {
        FD_CLR(fd, &m_rfds);
    }
    if ((event_type & GY_EVENT_WRITE) != 0)
    {
        FD_CLR(fd, &m_wfds);
    }
    if ((event_type & GY_EVENT_ERROR) != 0)
    {
        FD_CLR(fd, &m_efds);
    }
    if (this->m_maxfd == fd)
        this->m_maxfd--;
    else if (this->m_minfd == fd)
        this->m_minfd++;
    return 0;
}

int
galay::GY_SelectScheduler::ModEvent(int fd, int from ,int to)
{
    if(fd >= __FD_SETSIZE) return -1;
    int ret = DelEvent(fd, from);
    if(ret == -1) return ret;
    ret = AddEvent(fd, to);
    return ret;
}


int 
galay::GY_SelectScheduler::AddEvent(int fd, int event_type)
{
    if(fd >= __FD_SETSIZE) return -1;
    if ((event_type & GY_EVENT_READ) != 0)
    {
        FD_SET(fd, &m_rfds);
    }
    if ((event_type & GY_EVENT_WRITE) != 0)
    {
        FD_SET(fd, &m_wfds);
    }
    if ((event_type & GY_EVENT_ERROR) != 0)
    {
        FD_SET(fd, &m_efds);
    }
    this->m_maxfd = ::std::max(this->m_maxfd, fd);
    this->m_minfd = ::std::min(this->m_minfd, fd);
    return 0;
}


std::shared_ptr<galay::Timer>
galay::GY_SelectScheduler::AddTimer(uint64_t during, uint32_t exec_times, ::std::function<::std::any()> &&func)
{
    auto timerManager = std::dynamic_pointer_cast<GY_TimerManager>(m_objectors[m_timerfd]);
    return timerManager->AddTimer(during, exec_times, ::std::forward<::std::function<::std::any()>&&>(func));
}

void 
galay::GY_SelectScheduler::Start()
{
    fd_set read_set, write_set, excep_set;
    timeval tv;
    tv.tv_sec = this->m_builder->GetScheWaitTime() / 1000;
    tv.tv_usec = this->m_builder->GetScheWaitTime() % 1000 * 1000;
    while (1)
    {
        memcpy(&read_set, &m_rfds, sizeof(fd_set));
        memcpy(&write_set, &m_wfds, sizeof(fd_set));
        memcpy(&excep_set, &m_efds, sizeof(fd_set));
        int nready = select(this->m_maxfd + 1, &read_set, &write_set, &excep_set, &tv);
        if (this->IsStop())
            break;
        if (nready == -1)
        {
            spdlog::error("[{}:{}] [[select(tid: {})] [Errmsg:{}]]",__FILE__,__LINE__,::std::hash<::std::thread::id>{}(::std::this_thread::get_id()),strerror(errno));
            return;
        }
        if (nready == 0)
            continue;
        for (int fd = m_minfd; fd > 0 && fd <= m_maxfd; fd++)
        {
            int eventType = 0;
            if (FD_ISSET(fd, &read_set))
            {
                eventType |= EventType::GY_EVENT_READ;
            }
            if(FD_ISSET(fd, &write_set))
            {
                eventType |= EventType::GY_EVENT_WRITE;
            }
            if(FD_ISSET(fd, &excep_set))
            {
                eventType |= EventType::GY_EVENT_ERROR;
            }
            if(eventType != 0){
                auto objector = m_objectors[fd];
                objector->SetEventType(eventType);
                objector->ExecuteTask();
            }
        }
    } 
}

bool 
galay::GY_SelectScheduler::IsStop() 
{
    return this->m_stop;
}

void
galay::GY_SelectScheduler::Stop()
{
    if (!m_stop)
    {
        FD_ZERO(&m_rfds);
        FD_ZERO(&m_wfds);
        FD_ZERO(&m_efds);
        for (auto it = m_objectors.begin(); it != m_objectors.end(); ++it)
        {
            this->DelEvent(it->first, GY_EVENT_READ | GY_EVENT_WRITE | GY_EVENT_ERROR);
            spdlog::debug("[{}:{}] [socket(fd :{}) close]", __FILE__, __LINE__, it->first);
            close(it->first);
        }
        this->m_objectors.clear();
        this->m_stop = true;
    }
}

galay::GY_SelectScheduler::~GY_SelectScheduler()
{

}

galay::GY_EpollScheduler::GY_EpollScheduler(GY_TcpServerBuilderBase::ptr builder)
{
    this->m_epollfd = epoll_create(1);
    this->m_builder = builder;
    this->m_events = new epoll_event[builder->GetMaxEventSize()];
    this->m_stop = false;
    this->m_userFunc = builder->GetUserFunction();
    this->m_illegalFunc = builder->GetIllegalFunction();
}

galay::GY_TcpServerBuilderBase::wptr 
galay::GY_EpollScheduler::GetTcpServerBuilder() 
{
    return this->m_builder;
}

void 
galay::GY_EpollScheduler::RegisterObjector(int fd, ::std::shared_ptr<GY_Objector> objector)
{
    this->m_objectors[fd] = objector;
}

void 
galay::GY_EpollScheduler::RegiserTimerManager(int fd, ::std::shared_ptr<GY_TimerManager> timerManager)
{
    this->m_timerfd = fd;
    RegisterObjector(fd,timerManager);
    AddEvent(m_timerfd, EventType::GY_EVENT_ERROR | EventType::GY_EVENT_READ);
}

std::shared_ptr<galay::Timer> 
galay::GY_EpollScheduler::AddTimer(uint64_t during, uint32_t exec_times, ::std::function<::std::any()> &&func)
{
    auto timerManager = std::dynamic_pointer_cast<GY_TimerManager>(m_objectors[m_timerfd]);
    return timerManager->AddTimer(during, exec_times, ::std::forward<::std::function<::std::any()>&&>(func));
}

void 
galay::GY_EpollScheduler::DelObjector(int fd) 
{
    m_objectors.erase(fd);
    close(fd);
}
void 
galay::GY_EpollScheduler::Start() 
{
    uint16_t eventSize = m_builder->GetMaxEventSize() , waitTime = m_builder->GetScheWaitTime();
    while (1)
    {
        int nready = epoll_wait(this->m_epollfd, m_events, eventSize, waitTime);
        if(IsStop()) break;
        if(nready < 0){
            spdlog::error("[{}:{}] [[epoll_wait error(tid: {})] [Errmsg:{}]]",__FILE__,__LINE__,::std::hash<::std::thread::id>{}(::std::this_thread::get_id()),strerror(errno));
            return;
        }
        while(--nready >= 0){
            int eventType = 0;
            if(m_events[nready].events & EPOLLIN){
                eventType |= EventType::GY_EVENT_READ;
            }
            if(m_events[nready].events & EPOLLOUT){
                eventType |= EventType::GY_EVENT_WRITE;
            }
            if(m_events[nready].events & EPOLLERR){
                eventType |= EventType::GY_EVENT_ERROR;
            }
            if(eventType != 0){
                int fd = m_events[nready].data.fd;
                auto objector = m_objectors[fd];
                objector->SetEventType(eventType);
                objector->ExecuteTask();
            }
        }
    }
    
}

galay::GY_TcpCoroutine<galay::CoroutineStatus> 
galay::GY_EpollScheduler::UserFunction(galay::GY_Controller::ptr controller) 
{
    return this->m_userFunc(controller);
}

galay::GY_TcpCoroutine<galay::CoroutineStatus> 
galay::GY_EpollScheduler::IllegalFunction(std::string& rbuffer,std::string& wbuffer) 
{
    return this->m_illegalFunc(rbuffer,wbuffer);
}


int 
galay::GY_EpollScheduler::DelEvent(int fd, int event_type) 
{
    epoll_event ev = {0};
    ev.data.fd = fd;
    if((event_type & GY_EVENT_READ) != 0)
    {
        ev.events |= EPOLLIN;
    }
    if((event_type & GY_EVENT_WRITE) != 0)
    {
        ev.events |= EPOLLOUT;
    }
    if((event_type & GY_EVENT_ERROR) != 0)
    {
        ev.events |= EPOLLRDHUP;
    }
    return epoll_ctl(m_epollfd, EPOLL_CTL_DEL, fd, &ev);
}

int 
galay::GY_EpollScheduler::ModEvent(int fd, int from ,int to) 
{
    epoll_event ev = {0};
    ev.data.fd = fd;
    if((to & GY_EVENT_READ) != 0)
    {
        ev.events |= EPOLLIN;
    }
    if((to & GY_EVENT_WRITE) != 0)
    {
        ev.events |= EPOLLOUT;
    }
    if((to & GY_EVENT_ERROR) != 0)
    {
        ev.events |= EPOLLRDHUP;
    }
    return epoll_ctl(m_epollfd, EPOLL_CTL_MOD, fd, &ev);
}

int 
galay::GY_EpollScheduler::AddEvent(int fd, int event_type) 
{
    epoll_event ev = {0};
    ev.data.fd = fd;
    if((event_type & GY_EVENT_READ) != 0)
    {
        ev.events |= EPOLLIN;
    }
    if((event_type & GY_EVENT_WRITE) != 0)
    {
        ev.events |= EPOLLOUT;
    }
    if((event_type & GY_EVENT_ERROR) != 0)
    {
        ev.events |= EPOLLRDHUP;
    }
    if((event_type & GY_EVENT_EPOLLET) != 0)
    {
        ev.events |= EPOLLET;
    }
    if((event_type & GY_EVENT_EPOLLONESHOT ) != 0)
    {
        ev.events |= EPOLLONESHOT;
    }
    return epoll_ctl(m_epollfd, EPOLL_CTL_ADD, fd, &ev);
}
void 
galay::GY_EpollScheduler::Stop() 
{
    this->m_stop = true;
}
bool 
galay::GY_EpollScheduler::IsStop() 
{
    return this->m_stop;
}

galay::GY_EpollScheduler::~GY_EpollScheduler()
{
    close(this->m_epollfd);
    delete [] this->m_events;
}
