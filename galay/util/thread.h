#ifndef GALAY_THREADPOOL_H
#define GALAY_THREADPOOL_H

#include <thread>
#include <mutex>
#include <memory>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <functional>
#include <future>

namespace galay::thread
{
    class ThreadTask
    {
    public:
        using ptr = std::shared_ptr<ThreadTask>;
        ThreadTask(std::function<void()> &&func);
        void Execute();
        ~ThreadTask() = default;
    private:
        std::function<void()> m_func;
    };
    
    class ThreadWaiters
    {
    public:
        using ptr = std::shared_ptr<ThreadWaiters>;
        ThreadWaiters(int num);
        bool Wait(int timeout = -1); //ms
        bool Decrease();
    private:
        std::mutex m_mutex;
        std::atomic_int m_num;
        std::condition_variable m_cond;
    };

    class ThreadPool
    {
    private:
        void Run();
        void Done();
    public:
        using ptr = std::shared_ptr<ThreadPool>;
        using wptr = std::weak_ptr<ThreadPool>;
        using uptr = std::unique_ptr<ThreadPool>;
        
        ThreadPool();
        template <typename F, typename... Args>
        inline auto AddTask(F &&f, Args &&...args) -> std::future<decltype(f(args...))>;
        void Start(int num);
        bool WaitForAllDone(uint32_t timeout = 0);
        bool IsDone();
        void Stop();
        ~ThreadPool();

    protected:
        std::mutex m_mtx;
        std::queue<std::shared_ptr<ThreadTask>> m_tasks;  // 任务队列
        std::vector<std::unique_ptr<std::thread>> m_threads; // 工作线程
        std::condition_variable m_workCond; // worker
        std::condition_variable m_exitCond; 
        std::atomic_uint8_t m_nums;
        std::atomic_bool m_terminate;   // 结束线程池
        std::atomic_bool m_isDone;
    };

    template <typename F, typename... Args>
    inline auto ThreadPool::AddTask(F &&f, Args &&...args) -> std::future<decltype(f(args...))>
    {
        using RetType = decltype(f(args...));
        std::shared_ptr<std::packaged_task<RetType()>> func = std::make_shared<std::packaged_task<RetType()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        auto t_func = [func]()
        {
            (*func)();
        };
        ThreadTask::ptr task = std::make_shared<ThreadTask>(t_func);
        std::unique_lock<std::mutex> lock(m_mtx);
        m_tasks.push(task);
        lock.unlock();
        m_workCond.notify_one();
        return func->get_future();
    }

}




#endif