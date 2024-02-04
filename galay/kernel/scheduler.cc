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
    //不在构造函数构造，因为构造函数中Scheduler还未创建实例无法使用weak_ptr
    if(!m_timer_manager){
        this->m_timer_manager = std::make_shared<Timer_Manager>(shared_from_this());
        auto time_task = std::make_shared<Time_Task>(this->m_timer_manager);
        if(this->add_event(this->m_timer_manager->get_timerfd(), GY_EVENT_READ | GY_EVENT_EPOLLET| GY_EVENT_ERROR)==-1)
        {
            std::cout<< "add event failed fd = " <<this->m_timer_manager->get_timerfd() <<'\n';
        }
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
            std::cout<< error::scheduler_error::GY_SCHEDULER_ENGINE_CHECK_ERROR << ": "<< error::get_err_str(error::scheduler_error::GY_SCHEDULER_ENGINE_CHECK_ERROR) << '\n';
    //        std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }
        for (int i = 0; i < nready; i++)
        {
            if (this->m_tasks.contains(m_events[i].data.fd))
            {
                Task_Base::ptr task = this->m_tasks.at(m_events[i].data.fd);
                task->exec();
                if (task->is_destroy()) {
                    del_task(m_events[i].data.fd);
                    del_event(m_events[i].data.fd,GY_EVENT_READ|GY_EVENT_WRITE| GY_EVENT_ERROR);
                    close(m_events[i].data.fd);
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

int galay::Epoll_Scheduler::add_event(int fd , int event_type)
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
    return epoll_ctl(m_epfd, EPOLL_CTL_ADD, fd, &ev);
}

int galay::Epoll_Scheduler::del_event(int fd , int event_type)
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
    return epoll_ctl(m_epfd, EPOLL_CTL_DEL, fd, &ev);
}

int galay::Epoll_Scheduler::mod_event(int fd, int from , int to)
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
    return epoll_ctl(m_epfd, EPOLL_CTL_MOD, fd, &ev);
}

void galay::Epoll_Scheduler::stop()
{
    if (!this->m_stop)
    {
        this->m_stop = true;
        for (auto it = this->m_tasks.begin(); it != this->m_tasks.end(); it++)
        {
            this->del_event(it->first, GY_EVENT_READ | GY_EVENT_WRITE| GY_EVENT_ERROR);
            close(it->first);
            it->second.reset();
        }
        this->m_tasks.clear();
        close(this->m_epfd);
    }
}

galay::Timer_Manager::ptr galay::Epoll_Scheduler::get_timer_manager()
{
    if(!this->m_timer_manager)
    {
        this->m_timer_manager = std::make_shared<Timer_Manager>(shared_from_this());
        auto time_task = std::make_shared<Time_Task>(this->m_timer_manager);
        if(this->add_event(this->m_timer_manager->get_timerfd(), GY_EVENT_READ | GY_EVENT_EPOLLET| GY_EVENT_ERROR)==-1)
        {
            std::cout<< "add event failed fd = " <<this->m_timer_manager->get_timerfd() <<'\n';
        }
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
    std::unique_lock<std::mutex> lock(this->m_mtx);
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
    std::unique_lock<std::mutex> lock(this->m_mtx);
    auto it = this->m_tasks.find(fd);
    if (it != this->m_tasks.end()){
        this->m_tasks.erase(fd);
    }
}

galay::Epoll_Scheduler::~Epoll_Scheduler() 
{
    if(this->m_events){
        delete[] m_events;
        m_events = nullptr;
    }
}


//select scheduler
galay::Select_Scheduler::Select_Scheduler(int timeout) // ms
{
    FD_ZERO(&m_rfds);
    FD_ZERO(&m_wfds);
    FD_ZERO(&m_efds);
    this->m_stop = false;
    this->m_time_out = (timeout == -1 ? 0:timeout);
}

void galay::Select_Scheduler::add_task(std::pair<int, std::shared_ptr<Task_Base>> &&pair)
{
    std::unique_lock<std::mutex> lock(this->m_mtx);
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

void galay::Select_Scheduler::del_task(int fd)
{
    std::unique_lock<std::mutex> lock(this->m_mtx);
    auto it = this->m_tasks.find(fd);
    if (it != this->m_tasks.end())
    {
        this->m_tasks.erase(fd);
    }
}

int galay::Select_Scheduler::add_event(int fd, int event_type)
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
    this->m_maxfd = std::max(this->m_maxfd, fd);
    this->m_minfd = std::min(this->m_minfd, fd);
    return 0;
}

int galay::Select_Scheduler::del_event(int fd, int event_type)
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

int galay::Select_Scheduler::mod_event(int fd, int from, int to)
{
    if(fd >= __FD_SETSIZE) return -1;
    int ret = del_event(fd, from);
    if(ret == -1) return ret;
    ret = add_event(fd, to);
    return ret;
}

bool galay::Select_Scheduler::is_stop()
{
    return this->m_stop;
}

int galay::Select_Scheduler::start()
{
    fd_set read_set, write_set, excep_set;
    timeval tv;
    tv.tv_sec = this->m_time_out / 1000;
    tv.tv_usec = this->m_time_out % 1000 * 1000;
    std::unique_lock<std::mutex> lock(this->m_mtx, std::defer_lock);
    if (!m_timer_manager)
    {
        this->m_timer_manager = std::make_shared<Timer_Manager>(shared_from_this());
        auto time_task = std::make_shared<Time_Task>(this->m_timer_manager);
        if(this->add_event(this->m_timer_manager->get_timerfd(), GY_EVENT_READ | GY_EVENT_EPOLLET | GY_EVENT_ERROR)==-1)
        {
            std::cout<< "add event failed fd = " <<this->m_timer_manager->get_timerfd() <<'\n';
        }
        add_task({this->m_timer_manager->get_timerfd(), time_task});
    }
    while (1)
    {
        m_timer_manager->update_time();
        lock.lock();
        memcpy(&read_set, &m_rfds, sizeof(fd_set));
        memcpy(&write_set, &m_wfds, sizeof(fd_set));
        memcpy(&excep_set, &m_efds, sizeof(fd_set));
        lock.unlock();
        int nready = select(this->m_maxfd + 1, &read_set, &write_set, &excep_set, &tv);
        if (this->is_stop())
            break;
        if (nready == -1)
        {
            //std::cout << "select failed :" << error::GY_SCHEDULER_ENGINE_CHECK_ERROR << error::get_err_str(error::GY_SCHEDULER_ENGINE_CHECK_ERROR) << '\n';
            //continue;
            std::cout << strerror(errno) << '\n';
            break;
        }
        if (nready == 0)
            continue;
        for (int fd = m_minfd; fd > 0 && fd <= m_maxfd; fd++)
        {
            if (FD_ISSET(fd, &read_set) || FD_ISSET(fd, &write_set) || FD_ISSET(fd, &excep_set))
            {
                if (this->m_tasks.contains(fd))
                {
                    Task_Base::ptr task = this->m_tasks.at(fd);
                    task->exec();
                    if (task->is_destroy())
                    {
                        del_task(fd);
                        this->del_event(fd, GY_EVENT_READ | GY_EVENT_WRITE| GY_EVENT_ERROR);
                        close(fd);
                    }
                }
            }
        }
    }
    return 0;
}

std::shared_ptr<galay::Timer_Manager> galay::Select_Scheduler::get_timer_manager()
{
    if (!m_timer_manager)
    {
        this->m_timer_manager = std::make_shared<Timer_Manager>(shared_from_this());
        auto time_task = std::make_shared<Time_Task>(this->m_timer_manager);
        if(this->add_event(this->m_timer_manager->get_timerfd(), GY_EVENT_READ | GY_EVENT_EPOLLET | GY_EVENT_ERROR)==-1)
        {
            std::cout<< "add event failed fd = " <<this->m_timer_manager->get_timerfd() <<'\n';
        }
        add_task({this->m_timer_manager->get_timerfd(), time_task});
    }
    return this->m_timer_manager;
}

void galay::Select_Scheduler::stop()
{
    FD_ZERO(&m_rfds);
    FD_ZERO(&m_wfds);
    FD_ZERO(&m_efds);
    for(auto it = m_tasks.begin() ; it != m_tasks.end() ; it++)
    {
        this->del_event(it->first, GY_EVENT_READ | GY_EVENT_WRITE | GY_EVENT_ERROR);
        close(it->first);
        it->second.reset();
    }
    this->m_tasks.clear();
    this->m_stop = true;
}

galay::Select_Scheduler::~Select_Scheduler()
{
    if(!this->m_stop) stop();

}