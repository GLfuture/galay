#ifndef GALAY_COROUTINE_HPP
#define GALAY_COROUTINE_HPP

#include <memory>
#include <atomic>
#include <coroutine>
#include <mutex>
#include <list>
#include <optional>
#include <shared_mutex>
#include <string>
#include <functional>
#include <unordered_map>
#include "Scheduler.h"
#include "EventEngine.h"
#include "Internal.hpp"
#include <iostream>

namespace galay::details 
{

class CoroutineScheduler;

class WaitEvent;
class WaitAction;

}

namespace galay {


class CoroutineBase
{
public:
    using ptr = std::shared_ptr<CoroutineBase>;
    using wptr = std::weak_ptr<CoroutineBase>;
    virtual void Resume() const = 0;
    virtual bool Done() const = 0;
    virtual void Destroy() = 0;
    virtual details::CoroutineScheduler* BelongScheduler() const = 0;
    virtual bool SetAwaiter(AwaiterBase* awaiter) = 0; 
    virtual AwaiterBase* GetAwaiter() const = 0;
    virtual void AppendExitCallback(const std::function<void()>& callback) = 0;
    virtual ~CoroutineBase() = default;
};

template<typename T>
class Coroutine;

template<typename T>
class PromiseType
{
    friend class Coroutine<T>;
    using CoroutineSchedulerHolder = details::SchedulerHolder<details::RoundRobinLoadBalancer<details::CoroutineScheduler>>;
public:
    int get_return_object_on_alloaction_failure() noexcept { return -1; }
    Coroutine<T> get_return_object() noexcept;
    std::suspend_never initial_suspend() noexcept { return {}; }
    std::suspend_always yield_value(T&& value) noexcept;
    std::suspend_never final_suspend() noexcept { return {};  }
    void unhandled_exception() noexcept {}
    void return_value (T&& value) const noexcept;
    std::weak_ptr<Coroutine<T>> GetCoroutine() { return m_coroutine; }
    ~PromiseType();
private:
    std::shared_ptr<Coroutine<T>> m_coroutine;
};

template<>
class PromiseType<void>
{
    friend class Coroutine<void>;
    using CoroutineSchedulerHolder = details::SchedulerHolder<details::RoundRobinLoadBalancer<details::CoroutineScheduler>>;
public:
    int get_return_object_on_alloaction_failure() noexcept { return -1; }
    Coroutine<void> get_return_object() noexcept;
    std::suspend_never initial_suspend() noexcept { return {}; }
    static std::suspend_always yield_value() noexcept { return {}; }
    static std::suspend_never final_suspend() noexcept { return {};  }
    static void unhandled_exception() noexcept {}
    void return_void () const noexcept {};
    std::weak_ptr<Coroutine<void>> GetCoroutine() { return m_coroutine; }
    ~PromiseType();
private:
    std::shared_ptr<Coroutine<void>> m_coroutine;
};


template<typename T>
class Coroutine: public CoroutineBase
{
    friend class PromiseType<T>;
public:
    Coroutine& operator=(Coroutine& other) = delete;
    Coroutine& operator=(const Coroutine& other) = delete;
public:
    using ptr = std::shared_ptr<Coroutine>;
    using wptr = std::weak_ptr<Coroutine>;
    using promise_type = PromiseType<T>;
    
    explicit Coroutine(std::coroutine_handle<promise_type> handle, details::CoroutineScheduler* scheduler) noexcept;
    Coroutine(Coroutine&& other) noexcept;
    Coroutine(const Coroutine& other) noexcept;
    Coroutine& operator=(Coroutine&& other) noexcept;
    
    std::coroutine_handle<promise_type> GetHandle() const { return this->m_handle; }
    details::CoroutineScheduler* BelongScheduler() const override { return this->m_scheduler; }
    void Destroy() override { m_handle.destroy(); }
    bool Done() const  override { return *m_is_done; }
    void Resume() const override { return m_handle.resume(); }
    bool SetAwaiter(AwaiterBase* awaiter);
    AwaiterBase* GetAwaiter() const;

    void AppendExitCallback(const std::function<void()>& callback) override;
    std::optional<T> operator()() { return *m_result; }
    ~Coroutine() = default;
private:
    void ToExit();
private:
    std::shared_ptr<std::optional<T>> m_result;
    std::shared_ptr<std::atomic_bool> m_is_done;
    std::list<std::function<void()>> m_exit_cbs;
    std::coroutine_handle<promise_type> m_handle;
    std::atomic<AwaiterBase*> m_awaiter = nullptr;
    std::atomic<details::CoroutineScheduler*> m_scheduler;
};

template<>
class Coroutine<void>: public CoroutineBase
{
    friend class PromiseType<void>;
public:
    Coroutine& operator=(Coroutine& other) = delete;
    Coroutine& operator=(const Coroutine& other) = delete;
public:
    using ptr = std::shared_ptr<Coroutine>;
    using wptr = std::weak_ptr<Coroutine>;
    using promise_type = PromiseType<void>;
    
    explicit Coroutine(std::coroutine_handle<promise_type> handle, details::CoroutineScheduler* scheduler) noexcept;
    Coroutine(Coroutine&& other) noexcept;
    Coroutine(const Coroutine& other) noexcept;
    Coroutine& operator=(Coroutine&& other) noexcept;
    
    std::coroutine_handle<promise_type> GetHandle() const { return this->m_handle; }
    details::CoroutineScheduler* BelongScheduler() const override { return this->m_scheduler; }
    void Destroy() override { m_handle.destroy(); }
    bool Done() const  override { return *m_is_done; }
    void Resume() const override { return m_handle.resume(); }
    bool SetAwaiter(AwaiterBase* awaiter);
    AwaiterBase* GetAwaiter() const;

    void AppendExitCallback(const std::function<void()>& callback) override;
    void operator()() {}
    ~Coroutine() = default;
private:
    void ToExit();
private:
    std::shared_ptr<std::atomic_bool> m_is_done;
    std::list<std::function<void()>> m_exit_cbs;
    std::coroutine_handle<promise_type> m_handle;
    std::atomic<AwaiterBase*> m_awaiter = nullptr;
    std::atomic<details::CoroutineScheduler*> m_scheduler;
};

template<typename T>
class Awaiter: public AwaiterBase
{
public:
    Awaiter(details::WaitAction* action, void* ctx);
    Awaiter(T result);
    void SetResult(T result);
protected:
    void* m_ctx;
    T m_result;
    details::WaitAction* m_action;
};


template<typename T, typename CoRtn>
class AsyncResult: public Awaiter<T>
{
public:
    AsyncResult(details::WaitAction* action, void* ctx);
    AsyncResult(T result);
    bool await_ready() const noexcept;
    //true will suspend, false will not
    bool await_suspend(std::coroutine_handle<typename Coroutine<CoRtn>::promise_type> handle) noexcept;
    T await_resume() const noexcept;
    [[nodiscard]] Coroutine<CoRtn>::wptr GetCoroutine() const;
private:
    std::coroutine_handle<typename Coroutine<CoRtn>::promise_type> m_handle;
};

}

namespace galay::details
{

class WaitAction
{
public:
    virtual ~WaitAction() = default;
    virtual bool HasEventToDo() = 0;
    virtual bool DoAction(CoroutineBase::wptr co, void* ctx) = 0;
};

}

namespace galay
{


#define DEFAULT_COROUTINE_STORE_HASH_SIZE       2048

// class CoroutineStore
// {
// public:
//     CoroutineStore();
//     void Reserve(size_t size);
//     void AddCoroutine(std::weak_ptr<Coroutine> co);
//     void RemoveCoroutine(std::weak_ptr<Coroutine> co);
//     bool Exist(std::weak_ptr<Coroutine> co);
//     void Clear();
// private:
//     std::shared_mutex m_mutex;
//     std::unordered_map<Coroutine*, std::weak_ptr<Coroutine>> m_coroutines;
// };



}



#include "Coroutine.tcc"

#endif
