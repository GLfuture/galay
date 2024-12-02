#include "Thread.h"
#include "kernel/Log.h"

namespace galay::thread
{
ThreadTask::ThreadTask(std::function<void()>&& func)
{
    this->m_func = func;
}

void 
ThreadTask::Execute()
{
    this->m_func();
}

ThreadWaiters::ThreadWaiters(int num)
{
    this->m_num.store(num);
}

bool ThreadWaiters::Wait(int timeout)
{
    std::unique_lock lock(this->m_mutex);
    if(m_num.load() <= 0) return true;
    if(timeout == -1)
    {
        m_cond.wait(lock, [this]() {
            return m_num.load() <= 0;
        });
    }
    else
    {
        m_cond.wait_for(lock, std::chrono::milliseconds(timeout), [this]() {
            return m_num.load() == 0;
        });
        if(m_num.load() != 0) return false;
    }
    return true;
}

bool ThreadWaiters::Decrease()
{
    std::unique_lock lock(this->m_mutex);
    if( m_num.load() == 0) return false;
    m_num.fetch_sub(1);
    if(m_num.load() == 0)
    {
        m_cond.notify_one();
    }
    return true;
}

ScrambleThreadPool::ScrambleThreadPool()
{
    m_terminate.store(false, std::memory_order_relaxed);
    m_nums.store(0);
    m_isDone.store(false);
}

void 
ScrambleThreadPool::Run()
{
    while (!m_terminate.load())
    {
        std::unique_lock<std::mutex> lock(m_mtx);
        m_workCond.wait(lock, [this]()
                    { return !m_tasks.empty() || m_terminate.load() == true; });
        if (m_terminate.load())
            break;
        std::shared_ptr<ThreadTask> task = m_tasks.front();
        m_tasks.pop();
        lock.unlock();
        task->Execute();
    }
}

void 
ScrambleThreadPool::Start(int num)
{
    this->m_nums.store(num);
    for (int i = 0; i < num; i++)
    {
        auto th = std::make_unique<std::thread>([this](){
            Run();
            LogInfo("[Thread Exit Normally]");
            Done();
        });
        th->detach();
        m_threads.push_back(std::move(th));
    }
}

bool 
ScrambleThreadPool::WaitForAllDone(uint32_t timeout)
{
    std::mutex mtx;
    std::unique_lock<std::mutex> lock(mtx);
    if(timeout == 0){
        this->m_exitCond.wait(lock);
    }else{
        auto now = std::chrono::system_clock::now();
        this->m_exitCond.wait_for(lock, std::chrono::milliseconds(timeout));
        auto end = std::chrono::system_clock::now();
        if(std::chrono::duration_cast<std::chrono::milliseconds>(end - now) >= std::chrono::milliseconds(timeout)){
            if(this->m_nums.load() == 0) return true;
            else return false;
        }
    }
    return true;
}

bool 
ScrambleThreadPool::IsDone()
{
    return this->m_isDone.load();
}

void 
ScrambleThreadPool::Done()
{
    this->m_nums.fetch_sub(1);
    if(this->m_nums.load() == 0){
        this->m_exitCond.notify_one();
        this->m_isDone.store(true);
    }
}

void 
ScrambleThreadPool::Stop()
{
    if (!m_terminate.load())
    {
        m_terminate.store(true, std::memory_order_relaxed);
        m_workCond.notify_all();
    }
}

ScrambleThreadPool::~ScrambleThreadPool()
{
    if (!m_terminate.load())
    {
        Stop();
    }
}
}