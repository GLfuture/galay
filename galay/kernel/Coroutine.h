#ifndef GALAY_COROUTINE_H
#define GALAY_COROUTINE_H

#include <any>
#include <memory>
#include <atomic>
#include <coroutine>
#include <variant>
#include <string>
#include "../common/Base.h"
#include "../util/ThreadSefe.hpp"

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
class Coroutine;

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
    ~CoroutineStore();
private:
    thread::safe::List<Coroutine*> m_coroutines;
};

extern CoroutineStore g_coroutine_store;

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
        inline Coroutine get_return_object() noexcept {
            this->m_coroutine = new Coroutine(std::coroutine_handle<promise_type>::from_promise(*this));
            g_coroutine_store.AddCoroutine(m_coroutine);
            return *this->m_coroutine;
        }
        inline std::suspend_never initial_suspend() noexcept { return {}; }
        inline std::suspend_always yield_value() noexcept { return {}; }
        inline std::suspend_never final_suspend() noexcept { return {};  }
        inline void unhandled_exception() noexcept {}
        inline void return_void () noexcept { g_coroutine_store.RemoveCoroutine(m_coroutine); }
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
    void Destroy();
    inline bool Done() { return m_handle.done(); }
    inline void Resume() { return m_handle.resume(); }
    inline void SetAwaiter(Awaiter* awaiter) { m_awaiter = awaiter; }
    inline Awaiter* GetAwaiter() { return m_awaiter.load(); }
    inline thread::safe::ListNode<Coroutine*>*& GetListNode() { return m_node; }
    ~Coroutine() = default;
private:
    std::atomic<Awaiter*> m_awaiter;
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

class Awaiter_void;

extern Awaiter_void GetThisCoroutine(Coroutine*& coroutine);

}

#endif
