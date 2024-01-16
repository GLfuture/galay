#ifndef GALAY_THREADPOOL_H
#define GALAY_THREADPOOL_H

#include <iostream>
#include <thread>
#include <mutex>
#include <memory>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <functional>
#include <future>
#include "task.h"

namespace galay
{
    class ThreadPool
    {
    private:
        void run()
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

        void create(int num)
        {
            for (int i = 0; i < num; i++)
            {
                std::shared_ptr<std::thread> th = std::make_shared<std::thread>(&ThreadPool::run, this);
                m_threads.push_back(th);
            }
        }

        void wait_for_all_down()
        { // 等待任务全部结束
            for (int i = 0; i < m_threads.size(); i++)
            {
                if (m_threads[i] != NULL && m_threads[i]->joinable())
                { // 判断thread是否为空，防止destroy后调用析构函数重复销毁
                    m_threads[i]->join();
                }
            }
        }

    public:
        using ptr = std::shared_ptr<ThreadPool>;
        ThreadPool(int num)
        {
            m_terminate.store(false, std::memory_order_relaxed);
            m_threads.assign(0, NULL);
            create(num);
        }

        void reset(int num)
        {
            int temp = num - m_threads.size();
            if(temp > 0){
                create(temp);
            }else{
                for(int i = 0 ; i < -temp ; i++)
                {
                    m_threads.pop_back();
                }
            }
        }

        template <typename F, typename... Args>
        auto exec(F &&f, Args &&...args) -> std::future<decltype(f(args...))>
        {
            using RetType = decltype(f(args...));
            std::shared_ptr<std::packaged_task<RetType()>> func = std::make_shared<std::packaged_task<RetType()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
            auto t_func = [func]()
            {
                (*func)();
            };
            Thread_Task::ptr task = std::make_shared<Thread_Task>(t_func);
            std::unique_lock<std::mutex> lock(m_mtx);
            m_tasks.push(task);
            lock.unlock();
            m_cond.notify_one();
            return func->get_future();
        }

        void destroy()
        {
            if(!m_terminate.load())
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

        ~ThreadPool()
        {
            if(!m_terminate.load()) destroy();
        }

    protected:
        std::queue<std::shared_ptr<Thread_Task>> m_tasks;           // 任务队列
        std::vector<std::shared_ptr<std::thread>> m_threads; // 工作线程
        std::mutex m_mtx;
        std::condition_variable m_cond; // 条件变量
        std::atomic_bool m_terminate;   // 结束线程池
    };
}

#endif