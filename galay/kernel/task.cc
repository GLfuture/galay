#include "task.h"

//time task
galay::Time_Task::Time_Task(std::weak_ptr<Timer_Manager> manager)
{
    this->m_manager = manager;
}

int galay::Time_Task::exec()
{
    if (!m_manager.expired())
    {
        auto timer = m_manager.lock()->get_ealist_timer();
        if(timer && !timer->is_cancled()) timer->exec();
    }
    return 0;
}

galay::Time_Task::~Time_Task()
{

}

//thread task
galay::Thread_Task::Thread_Task(std::function<void()>&& func)
{
    this->m_func = func;
}

int galay::Thread_Task::exec()
{
    this->m_func();
    return 0;
}

galay::Thread_Task::~Thread_Task()
{

}