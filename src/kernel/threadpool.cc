#include "threadpool.h"

galay::ThreadPool::ThreadPool(int num)
{
    m_terminate.store(false, std::memory_order_relaxed);
    m_threads.assign(0, NULL);
    create(num);
}

void galay::ThreadPool::run()
{
    while (1)
    {
        std::unique_lock<std::mutex> lock(m_mtx);
        m_cond.wait(lock, [this]()
                    { return !m_tasks.empty() || m_terminate.load() == true; });
        if (m_terminate.load())
            break;
        std::shared_ptr<Thread_Task> task = m_tasks.front();
        m_tasks.pop();
        lock.unlock();
        task->exec();
    }
}

void galay::ThreadPool::create(int num)
{
    for (int i = 0; i < num; i++)
    {
        std::shared_ptr<std::thread> th = std::make_shared<std::thread>(&ThreadPool::run, this);
        m_threads.push_back(th);
    }
}

void galay::ThreadPool::wait_for_all_down()
{
    for (int i = 0; i < m_threads.size(); i++)
    {
        if (m_threads[i] != NULL && m_threads[i]->joinable())
        {
            m_threads[i]->join();
        }
    }
}

void galay::ThreadPool::reset(int num)
{
    int temp = num - m_threads.size();
    if (temp > 0)
    {
        create(temp);
    }
    else
    {
        for (int i = 0; i < -temp; i++)
        {
            m_threads.pop_back();
        }
    }
}

void galay::ThreadPool::destroy()
{
    if (!m_terminate.load())
    {
        m_terminate.store(true, std::memory_order_relaxed);
        m_cond.notify_all();
        wait_for_all_down();
        for (int i = 0; i < m_threads.size(); i++)
        {
            if (m_threads[i] != NULL)
                m_threads[i].reset();
        }
    }
}

galay::ThreadPool::~ThreadPool()
{
    if (!m_terminate.load())
        destroy();
}