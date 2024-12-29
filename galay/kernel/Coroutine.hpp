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

namespace galay::details 
{

class CoroutineScheduler;

class WaitEvent;
class WaitAction;

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

}

namespace galay 
{
template<typename T>
class PromiseType;

template<>
class PromiseType<void>;

class RoutineCtx
{
    template<typename CoRtn>
    friend class PromiseType;
    friend class PromiseType<void>;
    using CoroutineSchedulerHolder = details::SchedulerHolder<details::RoundRobinLoadBalancer<details::CoroutineScheduler>>;
public:
    RoutineCtx();
    RoutineCtx(details::CoroutineScheduler* scheduler);
    RoutineCtx(const RoutineCtx& ctx);
    RoutineCtx(RoutineCtx&& ctx);
    details::CoroutineScheduler* GetScheduler();
private:
    std::atomic<details::CoroutineScheduler*> m_scheduler;
};

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

class RoutineCtx;

template<typename T>
class PromiseType
{
    friend class Coroutine<T>;
public:
    template<typename ...Args>
    PromiseType(RoutineCtx ctx, Args&&... agrs);
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
    RoutineCtx m_ctx;
    std::shared_ptr<Coroutine<T>> m_coroutine;
};

template<>
class PromiseType<void>
{
    friend class Coroutine<void>;
public:
    template<typename ...Args>
    PromiseType(RoutineCtx ctx, Args&&... agrs);
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
    RoutineCtx m_ctx;
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
    
    explicit Coroutine(std::coroutine_handle<promise_type> handle) noexcept;
    Coroutine(Coroutine&& other) noexcept;
    Coroutine(const Coroutine& other) noexcept;
    Coroutine& operator=(Coroutine&& other) noexcept;
    
    details::CoroutineScheduler* BelongScheduler() const override;
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
    std::coroutine_handle<promise_type> m_handle;
    std::atomic<AwaiterBase*> m_awaiter = nullptr;
    std::shared_ptr<std::list<std::function<void()>>> m_exit_cbs;
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
    
    explicit Coroutine(std::coroutine_handle<promise_type> handle) noexcept;
    Coroutine(Coroutine&& other) noexcept;
    Coroutine(const Coroutine& other) noexcept;
    Coroutine& operator=(Coroutine&& other) noexcept;
    
    details::CoroutineScheduler* BelongScheduler() const override;
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
    std::coroutine_handle<promise_type> m_handle;
    std::atomic<AwaiterBase*> m_awaiter = nullptr;
    std::shared_ptr<std::list<std::function<void()>>> m_exit_cbs;
};




template<typename T, typename CoRtn>
class AsyncResult: public details::Awaiter<T>
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

template<typename CoRtn>
class AsyncResult<void, CoRtn>: public AwaiterBase
{
public:
    AsyncResult(details::WaitAction* action, void* ctx);
    AsyncResult() ;
    bool await_ready() const noexcept;
    //true will suspend, false will not
    bool await_suspend(std::coroutine_handle<typename Coroutine<CoRtn>::promise_type> handle) noexcept;
    void await_resume() const noexcept;
    [[nodiscard]] typename Coroutine<CoRtn>::wptr GetCoroutine() const;
private:
    void* m_ctx;
    details::WaitAction* m_action;
    std::coroutine_handle<typename Coroutine<CoRtn>::promise_type> m_handle;
};



}

template<typename CoRtn, typename ...Args>
struct std::coroutine_traits<galay::Coroutine<CoRtn>, Args...> {
    using promise_type = galay::Coroutine<CoRtn>::promise_type;
    static promise_type get_promise(galay::RoutineCtx ctx, Args&&... args) noexcept {
        return promise_type(ctx, std::forward<Args>(args)...);
    }
};

template<typename ...Args>
struct std::coroutine_traits<galay::Coroutine<void>, Args...> {
    using promise_type = galay::Coroutine<void>::promise_type;
    static promise_type get_promise(galay::RoutineCtx ctx, Args&&... args) noexcept {
        return promise_type(ctx, std::forward<Args>(args)...);
    }
};

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


#include "Coroutine.tcc"

#endif
