#ifndef GALAY_COROUTINE_H
#define GALAY_COROUTINE_H

#include <any>
#include <memory>
#include <atomic>
#include <coroutine>
#include <variant>
#include <string>
#include "galay/common/Base.h"
#include "galay/util/ThreadSefe.hpp"

namespace galay::action
{
    class CoroutineWaitAction;
}

namespace galay::scheduler
{
    class CoroutineScheduler;
}

namespace galay::event
{
    class Timer;
}

namespace galay::coroutine
{
class Coroutine;
class Awaiter_void;
class Awaiter_bool; 

class Awaiter
{
public:
    virtual void SetResult(const std::variant<std::monostate, int, bool, void*, std::string, GHandle>& result) = 0;
};

class CoroutineStore
{
public:
    void AddCoroutine(Coroutine* co);
    void RemoveCoroutine(Coroutine* co);
    void Clear();
private:
    thread::safe::List<Coroutine*> m_coroutines;
};

//如果上一个协程还在执行过程中没有到达暂停点，resume会从第一个暂停点重新执行
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
        Coroutine get_return_object() noexcept;
        inline std::suspend_never initial_suspend() noexcept { return {}; }
        inline std::suspend_always yield_value() noexcept { return {}; }
        inline std::suspend_never final_suspend() noexcept { return {};  }
        inline void unhandled_exception() noexcept {}
        void return_void () noexcept;
        inline Coroutine* GetCoroutine() { return m_coroutine; }
        inline CoroutineStore* GetStore() { return m_store; }
        inline ~promise_type() { delete m_coroutine; }
    private:
        CoroutineStore* m_store;
        Coroutine* m_coroutine;
    };
    Coroutine(std::coroutine_handle<promise_type> handle) noexcept;
    Coroutine(Coroutine&& other) noexcept;
    Coroutine(const Coroutine& other) noexcept;
    Coroutine& operator=(Coroutine&& other) noexcept;
    
    inline std::coroutine_handle<promise_type> GetHandle() { return this->m_handle; }
    void Destroy();
    inline bool Done() { return m_handle.done(); }
    inline void Resume() { return m_handle.resume(); }
    bool SetAwaiter(Awaiter* awaiter);
    Awaiter* GetAwaiter();
    inline thread::safe::ListNode<Coroutine*>*& GetListNode() { return m_node; }
    ~Coroutine() = default;
private:
    std::atomic<Awaiter*> m_awaiter = nullptr;
    thread::safe::ListNode<Coroutine*>* m_node;
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


namespace galay::this_coroutine
{
    extern coroutine::Awaiter_void GetThisCoroutine(coroutine::Coroutine*& coroutine);
    /*
        return false only when TimeScheduler is not running
        [ms] : ms
        [timer] : timer
        [scheduler] : coroutine_scheduler, this coroutine will resume at this scheduler
    */
    extern coroutine::Awaiter_bool SleepFor(int64_t ms, std::shared_ptr<event::Timer>* timer, scheduler::CoroutineScheduler* scheduler = nullptr);
}

#endif
