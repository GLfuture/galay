#include "threadpool.h"
#include <spdlog/spdlog.h>

galay::common::GY_ThreadTask::GY_ThreadTask(std::function<void()>&& func)
{
    this->m_func = func;
}

void 
galay::common::GY_ThreadTask::Execute()
{
    this->m_func();
}

galay::common::GY_ThreadTask::~GY_ThreadTask()
{

}

galay::common::GY_ThreadPool::GY_ThreadPool()
{
    m_terminate.store(false, std::memory_order_relaxed);
    m_threads.assign(0, NULL);
    m_nums.store(0);
}

void galay::common::GY_ThreadPool::Run()
{
    while (!m_terminate.load())
    {
        std::unique_lock<std::mutex> lock(m_mtx);
        m_workCond.wait(lock, [this]()
                    { return !m_tasks.empty() || m_terminate.load() == true; });
        if (m_terminate.load())
            break;
        std::shared_ptr<GY_ThreadTask> task = m_tasks.front();
        m_tasks.pop();
        lock.unlock();
        task->Execute();
    }
}

void galay::common::GY_ThreadPool::Start(int num)
{
    this->m_nums.store(num);
    for (int i = 0; i < num; i++)
    {
        std::shared_ptr<std::thread> th = std::make_shared<std::thread>([this](){
            Run();
            spdlog::info("[{}:{}] [Thread Exit Normally]",__FILE__,__LINE__);
            Done();
        });
        th->detach();
        m_threads.push_back(th);
    }
}

void galay::common::GY_ThreadPool::WaitForAllDown()
{
    std::mutex mtx;
    std::unique_lock<std::mutex> lock(mtx);
    this->m_exitCond.wait(lock);
}

void 
galay::common::GY_ThreadPool::Done()
{
    this->m_nums.fetch_sub(1);
    if(this->m_nums.load() == 0){
        this->m_exitCond.notify_one();
    }
}

void 
galay::common::GY_ThreadPool::Destory()
{
    if (!m_terminate.load())
    {
        m_terminate.store(true, std::memory_order_relaxed);
        m_workCond.notify_all();
    }
}

galay::common::GY_ThreadPool::~GY_ThreadPool()
{
    if (!m_terminate.load())
    {
        Destory();
    }
}