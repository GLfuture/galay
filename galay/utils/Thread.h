#ifndef GALAY_THREAD_H
#define GALAY_THREAD_H

#include <thread>
#include <mutex>
#include <memory>
#include <shared_mutex>
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

class ScrambleThreadPool
{
private:
    void Run();
    void Done();
public:
    using ptr = std::shared_ptr<ScrambleThreadPool>;
    using wptr = std::weak_ptr<ScrambleThreadPool>;
    using uptr = std::unique_ptr<ScrambleThreadPool>;
    
    ScrambleThreadPool();
    template <typename F, typename... Args>
    inline auto AddTask(F &&f, Args &&...args) -> std::future<decltype(f(args...))>;
    void Start(int num);
    bool WaitForAllDone(uint32_t timeout = 0);
    bool IsDone();
    void Stop();
    ~ScrambleThreadPool();

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
inline auto ScrambleThreadPool::AddTask(F &&f, Args &&...args) -> std::future<decltype(f(args...))>
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

namespace galay::thread
{

template<typename T>
struct ListNode
{
    ListNode() 
        :m_prev(nullptr), m_next(nullptr) {}
    ListNode(const T& data)
        :m_prev(nullptr), m_next(nullptr), m_data(data)  {}
    ListNode(T&& data)
        :m_prev(nullptr), m_next(nullptr), m_data(std::forward<T>(data)) {}
    ListNode* m_prev;
    ListNode* m_next;
    T m_data;
};

template<typename T>
class List
{
    static_assert(std::is_default_constructible_v<T>, "T ust be default constructible");
public:
    List()
    {
        m_head = new ListNode<T>();
        m_tail = new ListNode<T>();
        m_head->m_next = m_tail;
        m_tail->m_prev = m_head;
        m_size.store(0);
    }

    ListNode<T>* PushFront(const T& data)
    {
        std::unique_lock lock(this->m_mtx);
        ListNode<T>* node = new ListNode<T>(data);
        node->m_next = m_head->m_next;
        node->m_prev = m_head;
        m_head->m_next->m_prev = node;
        m_head->m_next = node;
        m_size.fetch_add(1);
        return node;
    }

    ListNode<T>* PushFront(T&& data)
    {
        std::unique_lock lock(this->m_mtx);
        ListNode<T>* node = new ListNode<T>(std::forward<T>(data));
        node->m_next = m_head->m_next;
        node->m_prev = m_head;
        m_head->m_next->m_prev = node;
        m_head->m_next = node;
        m_size.fetch_add(1);
        return node;
    }

    ListNode<T>* PushBack(const T& data)
    {
        std::unique_lock lock(this->m_mtx);
        ListNode<T>* node = new ListNode<T>(data);
        node->m_next = m_tail;
        node->m_prev = m_tail->m_prev;
        m_tail->m_prev->m_next = node;
        m_tail->m_prev = node;
        m_size.fetch_add(1);
        return node;
    }

    ListNode<T>* PushBack(T&& data)
    {
        std::unique_lock lock(this->m_mtx);
        ListNode<T>* node = new ListNode<T>(std::forward<T>(data));
        node->m_next = m_tail;
        node->m_prev = m_tail->m_prev;
        m_tail->m_prev->m_next = node;
        m_tail->m_prev = node;
        m_size.fetch_add(1);
        return node;
    }

    ListNode<T>* PopFront()
    {
        std::unique_lock lock(this->m_mtx);
        if( m_size.load() <= 0 ) return nullptr;
        ListNode<T>* node = m_head->m_next;
        node->m_next->m_prev = m_head;
        m_head->m_next = node->m_next;
        m_size.fetch_sub(1);
        node->m_next = nullptr;
        node->m_prev = nullptr;
        return node;
    }

    ListNode<T>* PopBack()
    {
        std::unique_lock lock(this->m_mtx);
        if( m_size.load() == 0 ) return nullptr;
        ListNode<T>* node = m_tail->m_prev;
        node->m_prev->m_next = m_tail;
        m_tail->m_prev = node->m_prev;
        m_size.fetch_sub(1);
        node->m_next = nullptr;
        node->m_prev = nullptr;
        return node;
    }
    

    bool Remove(ListNode<T>* node)
    {
        std::unique_lock lock(this->m_mtx);
        if( node == nullptr || node->m_next == nullptr || node->m_prev == nullptr ) return false;
        node->m_prev->m_next = node->m_next;
        node->m_next->m_prev = node->m_prev;
        m_size.fetch_sub(1);
        lock.unlock();
        delete node;
        return true;
    }

    bool Empty()
    {
        return m_size.load() == 0;
    }
    
    uint64_t Size()
    {
        return m_size.load();
    }

    ~List()
    {
        ListNode<T>* node = m_head;
        while(node)
        {
            ListNode<T>* temp = node;
            node = node->m_next;
            delete temp;
        }
    }
private:
    std::atomic_uint64_t m_size;
    ListNode<T>* m_head;
    ListNode<T>* m_tail;
    std::shared_mutex m_mtx;
};

}


#endif