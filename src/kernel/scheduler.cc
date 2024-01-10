#include "scheduler.h"

galay::IO_Scheduler::IO_Scheduler(IO_ENGINE engine_type, int max_event, int timeout)
{
    switch (engine_type)
    {
    case IO_ENGINE::IO_POLL:
    {
        break;
    }
    case IO_ENGINE::IO_SELECT:
        break;
    case IO_ENGINE::IO_EPOLL:
    {
        this->m_engine = std::make_shared<Epoll_Engine>(max_event, timeout);
        break;
    }
    case IO_ENGINE::IO_URING:
        break;
    }
    this->m_tasks = std::make_shared<std::unordered_map<int, std::shared_ptr<Task_Base>>>();
}

int galay::IO_Scheduler::start()
{
    while (1)
    {
        int ret = this->m_engine->event_check();
        if (this->m_stop)
            break;
        if (ret == -1)
        {
            return error::scheduler_error::GY_SCHEDULER_ENGINE_CHECK_ERROR;
        }
        epoll_event *events = (epoll_event *)this->m_engine->result();
        int nready = this->m_engine->get_active_event_num();
        for (int i = 0; i < nready; i++)
        {
            if (this->m_tasks->contains(events[i].data.fd))
            {
                Task_Base::ptr task = this->m_tasks->at(events[i].data.fd);
                task->exec();
                if (task->is_need_to_destroy())
                {
                    m_tasks->erase(events[i].data.fd);
                    m_engine->del_event(events[i].data.fd, EPOLLIN | EPOLLOUT);
                    close(events[i].data.fd);
                }
            }
        }
    }
    return error::base_error::GY_SUCCESS;
}

void galay::IO_Scheduler::stop()
{
    if (!this->m_stop)
    {
        this->m_stop = true;
        for (auto it = this->m_tasks->begin(); it != this->m_tasks->end(); it++)
        {
            this->m_engine->del_event(it->first, EPOLLIN);
            close(it->first);
            it->second.reset();
        }
        this->m_tasks->clear();
        this->m_engine->stop();
    }
}