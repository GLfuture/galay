#include "engine.h"

galay::Epoll_Engine::Epoll_Engine(int max_events,int time_out)
{
    m_epfd = epoll_create(1);
    m_events = new epoll_event[max_events];
    m_events_size = max_events;
    m_time_out = time_out;
}

int galay::Epoll_Engine::event_check()
{
    this->m_active_event_num = epoll_wait(m_epfd, m_events, m_events_size, m_time_out);
    if(this->m_active_event_num == -1){
        this->m_error = error::engine_error::GY_ENGINE_EPOLL_WAIT_ERROR;
        return -1;
    }
    return 0;
}

int galay::Epoll_Engine::get_epoll_fd() const
{
    return m_epfd;
}

void* galay::Epoll_Engine::result()
{
    return m_events;
}

int galay::Epoll_Engine::get_event_size() const 
{
    return m_events_size;
}

int galay::Epoll_Engine::get_active_event_num() const
{
    return m_active_event_num;
}

int galay::Epoll_Engine::add_event(int fd , uint32_t event_type)
{
    epoll_event ev{
        .events = event_type,
        .data{
            .fd = fd}};
    return epoll_ctl(m_epfd, EPOLL_CTL_ADD, fd, &ev);
}

int galay::Epoll_Engine::del_event(int fd , uint32_t event_type)
{
    epoll_event ev{
        .events = event_type,
        .data{
            .fd = fd}};
    return epoll_ctl(m_epfd, EPOLL_CTL_DEL, fd, &ev);
}

int galay::Epoll_Engine::mod_event(int fd, uint32_t event_type)
{
    epoll_event ev{
        .events = event_type,
        .data{
            .fd = fd}};
    return epoll_ctl(m_epfd, EPOLL_CTL_MOD, fd, &ev);
}

void galay::Epoll_Engine::stop()
{
    close(m_epfd);
}

galay::Epoll_Engine::~Epoll_Engine()
{
    if (m_events)
    {
        delete[] m_events;
        m_events = nullptr;
    }
}
