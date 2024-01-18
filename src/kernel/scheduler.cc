#include "scheduler.h"
#include "task.h"

galay::Epoll_Scheduler::Epoll_Scheduler(int max_event, int timeout)
{
    this->m_epfd = epoll_create(1);
    this->m_time_out = timeout;
    this->m_events_size = max_event;
    this->m_events = new epoll_event[max_event];
}

int galay::Epoll_Scheduler::start()
{
    if(!m_timer_manager){
        this->m_timer_manager = std::make_shared<Timer_Manager>(shared_from_this());
        auto time_task = std::make_shared<Time_Task>(this->m_timer_manager);
        this->add_event(this->m_timer_manager->get_timerfd(), EPOLLIN);
        add_task({this->m_timer_manager->get_timerfd(), time_task});
    }
    while (1)
    {
        this->m_timer_manager->update_time();
        int nready = epoll_wait(m_epfd, m_events, m_events_size, m_time_out);
        if (this->m_stop)
            break;
        if (nready == -1)
        {
            return error::scheduler_error::GY_SCHEDULER_ENGINE_CHECK_ERROR;
        }
        for (int i = 0; i < nready; i++)
        {
            if (this->m_tasks.contains(m_events[i].data.fd))
            {
                Task_Base::ptr task = this->m_tasks.at(m_events[i].data.fd);
                task->exec();
                if (task->is_destroy()) {
                    std::cout<< "del :" << m_events[i].data.fd <<'\n';
                    del_task(m_events[i].data.fd);
                }
            }
        }
    }
    return error::base_error::GY_SUCCESS;
}

int galay::Epoll_Scheduler::get_epoll_fd() const
{
    return m_epfd;
}


int galay::Epoll_Scheduler::get_event_size() const 
{
    return m_events_size;
}

int galay::Epoll_Scheduler::add_event(int fd , uint32_t event_type)
{
    epoll_event ev{
        .events = event_type,
        .data{
            .fd = fd}};
    return epoll_ctl(m_epfd, EPOLL_CTL_ADD, fd, &ev);
}

int galay::Epoll_Scheduler::del_event(int fd , uint32_t event_type)
{
    epoll_event ev{
        .events = event_type,
        .data{
            .fd = fd}};
    return epoll_ctl(m_epfd, EPOLL_CTL_DEL, fd, &ev);
}

int galay::Epoll_Scheduler::mod_event(int fd, uint32_t event_type)
{
    epoll_event ev{
        .events = event_type,
        .data{
            .fd = fd}};
    return epoll_ctl(m_epfd, EPOLL_CTL_MOD, fd, &ev);
}

void galay::Epoll_Scheduler::stop()
{
    if (!this->m_stop)
    {
        this->m_stop = true;
        for (auto it = this->m_tasks.begin(); it != this->m_tasks.end(); it++)
        {
            this->del_event(it->first, EPOLLIN);
            close(it->first);
            it->second.reset();
        }
        this->m_tasks.clear();
        this->stop();
    }
}

galay::Timer_Manager::ptr galay::Epoll_Scheduler::get_timer_manager()
{
    if(!this->m_timer_manager)
    {
        this->m_timer_manager = std::make_shared<Timer_Manager>(shared_from_this());
        auto time_task = std::make_shared<Time_Task>(this->m_timer_manager);
        this->add_event(this->m_timer_manager->get_timerfd(), EPOLLIN);
        add_task({this->m_timer_manager->get_timerfd(), time_task});
    }
    return this->m_timer_manager;
}

bool galay::Epoll_Scheduler::is_stop()
{
    return this->m_stop;
}

void galay::Epoll_Scheduler::add_task(std::pair<int,std::shared_ptr<Task_Base>>&& pair)
{
    std::unique_lock<std::shared_mutex> lock(this->m_mtx);
    auto it = this->m_tasks.find(pair.first);
    if (it == this->m_tasks.end())
    {
        this->m_tasks.emplace(pair);
    }
    else
    {
        it->second = pair.second;
    }
}

void galay::Epoll_Scheduler::del_task(int fd)
{
    std::unique_lock<std::shared_mutex> lock(this->m_mtx);
    auto it = this->m_tasks.find(fd);
    if (it != this->m_tasks.end()){
        it->second->destory();
        this->m_tasks.erase(fd);
        this->del_event(fd, EPOLLIN | EPOLLOUT);
        close(fd);
    }
}

galay::Epoll_Scheduler::~Epoll_Scheduler() 
{
    if(this->m_events){
        delete[] m_events;
        m_events = nullptr;
    }
}