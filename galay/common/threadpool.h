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

namespace galay
{
    namespace common
    {
        class GY_ThreadTask
        {
        public:
            using ptr = std::shared_ptr<GY_ThreadTask>;
            GY_ThreadTask(std::function<void()> &&func);
            void Execute();
            ~GY_ThreadTask();

        private:
            std::function<void()> m_func;
        };

        class ThreadPool
        {
        private:
            void run();

            void create(int num);

            void wait_for_all_down();

        public:
            using ptr = std::shared_ptr<ThreadPool>;
            ThreadPool(int num);
            void reset(int num);
            template <typename F, typename... Args>
            inline auto Exec(F &&f, Args &&...args) -> std::future<decltype(f(args...))>
            {
                using RetType = decltype(f(args...));
                std::shared_ptr<std::packaged_task<RetType()>> func = std::make_shared<std::packaged_task<RetType()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
                auto t_func = [func]()
                {
                    (*func)();
                };
                GY_ThreadTask::ptr task = std::make_shared<GY_ThreadTask>(t_func);
                std::unique_lock<std::mutex> lock(m_mtx);
                m_tasks.push(task);
                lock.unlock();
                m_cond.notify_one();
                return func->get_future();
            }

            void destroy();

            ~ThreadPool();

        protected:
            std::queue<std::shared_ptr<GY_ThreadTask>> m_tasks;  // 任务队列
            std::vector<std::shared_ptr<std::thread>> m_threads; // 工作线程
            std::mutex m_mtx;
            std::condition_variable m_cond; // 条件变量
            std::atomic_bool m_terminate;   // 结束线程池
        };
    }
}

#endif