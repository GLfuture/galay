#ifndef GALAY_COROUTINE_H
#define GALAY_COROUTINE_H

#include <any>
#include <memory>
#include <atomic>
#include <coroutine>

namespace galay::action
{
    class CoroutineWaitAction;
}

namespace galay::scheduler
{
    class CoroutineScheduler;
}

namespace galay::coroutine
{
    
class Coroutine
{
public:
    Coroutine& operator=(Coroutine& other) = delete;
    Coroutine& operator=(const Coroutine& other) = delete;
public:
    class promise_type
    {
    public:
        inline int get_return_object_on_alloaction_failure() noexcept { return -1; }
        inline Coroutine get_return_object() noexcept {
            this->m_coroutine = new Coroutine(std::coroutine_handle<promise_type>::from_promise(*this));
            return *this->m_coroutine;
        }
        inline std::suspend_never initial_suspend() noexcept { return {}; }
        inline std::suspend_always yield_value() noexcept { return {}; }
        inline std::suspend_never final_suspend() noexcept { return {};  }
        inline void unhandled_exception() noexcept {}
        inline void return_void () noexcept {}
        inline Coroutine* GetCoroutine() { return m_coroutine; }
        inline ~promise_type() { delete m_coroutine; }
    private:
        Coroutine* m_coroutine;
    };
    Coroutine(std::coroutine_handle<promise_type> handle) noexcept;
    Coroutine(Coroutine&& other) noexcept;
    Coroutine(const Coroutine& other) noexcept;
    Coroutine& operator=(Coroutine&& other) noexcept;
    
    inline std::coroutine_handle<promise_type> GetHandle() { return this->m_handle; }
    inline bool Done() { return m_handle.done(); }
    inline void Resume() { return m_handle.resume(); }
    inline void SetContext(std::any context) { *this->m_context = context; }
    inline std::any GetContext() { return *this->m_context; }
    ~Coroutine() = default;
private:
    std::shared_ptr<std::any> m_context;
    std::coroutine_handle<promise_type> m_handle;
};

class Awaiter_bool;

class CoroutineWaiters
{
public:
    using ptr = std::shared_ptr<CoroutineWaiters>;
    CoroutineWaiters(int num, scheduler::CoroutineScheduler* scheduler);
    Awaiter_bool Wait(int timeout = -1); //ms
    inline int GetRemainNum() const { return m_num.load(); };
    inline void AddCoroutineTaskNum(int num) { this->m_num.fetch_add(num); }
    bool Decrease();
    ~CoroutineWaiters();
private:
    scheduler::CoroutineScheduler* m_scheduler;
    std::atomic_int m_num;
    action::CoroutineWaitAction* m_action;
};

class CoroutineWaitContext
{
public:
    CoroutineWaitContext(CoroutineWaiters& waiters);
    void AddWaitCoroutineNum(int num);
    inline int GetRemainWaitCoroutine() const { return m_waiters.GetRemainNum(); };
    bool Done();
private:
    CoroutineWaiters& m_waiters;
};


}

#endif
