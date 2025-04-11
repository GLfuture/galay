#ifndef GALAY_COROUTINE_HPP
#define GALAY_COROUTINE_HPP

#include <memory>
#include <atomic>
#include <coroutine>
#include <mutex>
#include <list>
#include <variant>
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
    Awaiter(T&& result);
    void SetResult(T&& result);
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

class CoroutineBase;
class RoutineCtx;

#define COUROUTINE_TREE_DEPTH   5

class RoutineSharedCtx
{
    using CoroutineSchedulerHolder = details::SchedulerHolder<details::RoundRobinLoadBalancer<details::CoroutineScheduler>>;
public:
    using ptr = std::shared_ptr<RoutineSharedCtx>;
    using wptr = std::weak_ptr<RoutineSharedCtx>;

    static RoutineSharedCtx::ptr Create(details::EventScheduler* src_scheduler);
    RoutineSharedCtx(details::EventScheduler* src_scheduler, details::CoroutineScheduler* dest_scheduler);
    RoutineSharedCtx(const RoutineSharedCtx& ctx);
    RoutineSharedCtx(RoutineSharedCtx&& ctx);
    details::CoroutineScheduler* GetCoScheduler();
    details::EventScheduler* GetEventScheduler();
private:
    std::atomic<details::EventScheduler*> m_src_scheduler;
    std::atomic<details::CoroutineScheduler*> m_dest_scheduler;
};

class RoutinePrivateCtx
{
    friend class RoutineCtx;
public:
    using ptr = std::shared_ptr<RoutinePrivateCtx>;
    static RoutinePrivateCtx::ptr Create();
    RoutinePrivateCtx();
private:
};

class RoutineCtx
{
    template<typename CoRtn>
    friend class PromiseType;
    friend class PromiseType<void>;
public:
    static RoutineCtx Create(details::EventScheduler* src_scheduler);
    static RoutineCtx Create(details::EventScheduler* src_scheduler, details::CoroutineScheduler* dest_scheduler);

    RoutineCtx(RoutineSharedCtx::ptr sharedData);
    RoutineCtx(const RoutineCtx& ctx);
    RoutineCtx(RoutineCtx&& ctx);

    RoutineCtx Copy();

    RoutineSharedCtx::wptr GetSharedCtx();
private:
    RoutineSharedCtx::ptr m_sharedCtx;
    RoutinePrivateCtx::ptr m_privateCtx;
};

class CoroutineBase: public std::enable_shared_from_this<CoroutineBase>
{
public:
    using ptr = std::shared_ptr<CoroutineBase>;
    using wptr = std::weak_ptr<CoroutineBase>;

    using ExitHandle = std::function<void(CoroutineBase::wptr)>;
    
    virtual bool IsRunning() const = 0;
    virtual bool IsSuspend() const = 0;
    virtual bool IsDone() const = 0;
    virtual details::CoroutineScheduler* GetCoScheduler() const = 0;
    virtual details::EventScheduler* GetEventScheduler() const = 0;
    virtual AwaiterBase* GetAwaiter() const = 0;
    virtual void AppendExitCallback(const ExitHandle& callback) = 0;
    virtual bool SetAwaiter(AwaiterBase* awaiter) = 0; 
    virtual void Resume() = 0;
    virtual void Destroy() = 0;
    virtual ~CoroutineBase() = default;

    template <typename T>
    T* ConvertTo();
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
    std::suspend_always yield_value() noexcept;
    std::suspend_never final_suspend() noexcept { return {};  }
    void unhandled_exception() noexcept {}
    void return_void () const noexcept {};
    std::weak_ptr<Coroutine<void>> GetCoroutine() { return m_coroutine; }
    ~PromiseType();
private:
    RoutineCtx m_ctx;
    std::shared_ptr<Coroutine<void>> m_coroutine;
};

enum class CoroutineStatus: int32_t {
    Running = 0,
    Suspended,
    Finished,
};

template<typename T>
class Coroutine: public CoroutineBase
{
    friend class PromiseType<T>;
    friend class CoroutineScheduler;
public:
    using ptr = std::shared_ptr<Coroutine>;
    using wptr = std::weak_ptr<Coroutine>;
    using promise_type = PromiseType<T>;
    
    explicit Coroutine(std::coroutine_handle<promise_type> handle) noexcept;
    Coroutine(Coroutine&& other) noexcept;
    Coroutine(const Coroutine& other) noexcept;
    Coroutine& operator=(Coroutine&& other) noexcept;
    Coroutine& operator=(const Coroutine& other) noexcept;
    
    details::CoroutineScheduler* GetCoScheduler() const override;
    details::EventScheduler* GetEventScheduler() const override;
    bool IsRunning() const override;
    bool IsSuspend() const override;
    bool IsDone() const override;
    bool SetAwaiter(AwaiterBase* awaiter) override;
    AwaiterBase* GetAwaiter() const override;
    void AppendExitCallback(const ExitHandle& callback) override;

    std::optional<T> Result();
    std::optional<T> operator()();

    bool Become(CoroutineStatus status);
    ~Coroutine() = default;
private:
    void Destroy() override;
    void Resume() override;
    void ToExit();
private:
    std::shared_ptr<std::optional<T>> m_result;
    std::shared_ptr<std::atomic<CoroutineStatus>> m_status;
    std::coroutine_handle<promise_type> m_handle;
    std::atomic<AwaiterBase*> m_awaiter = nullptr;
    std::shared_ptr<std::list<ExitHandle>> m_exit_cbs;
};

template<>
class Coroutine<void>: public CoroutineBase
{
    friend class PromiseType<void>;
    friend class CoroutineScheduler;
public:
    using ptr = std::shared_ptr<Coroutine>;
    using wptr = std::weak_ptr<Coroutine>;
    using promise_type = PromiseType<void>;
    
    explicit Coroutine(std::coroutine_handle<promise_type> handle) noexcept;
    Coroutine(Coroutine&& other) noexcept;
    Coroutine(const Coroutine& other) noexcept;
    Coroutine& operator=(Coroutine&& other) noexcept;
    Coroutine& operator=(const Coroutine& other) noexcept;
    
    details::CoroutineScheduler* GetCoScheduler() const override;
    details::EventScheduler* GetEventScheduler() const override;
    bool IsRunning() const override;
    bool IsSuspend() const override;
    bool IsDone() const  override;
    bool SetAwaiter(AwaiterBase* awaiter) override;
    AwaiterBase* GetAwaiter() const override;
    void AppendExitCallback(const ExitHandle& callback) override;

    std::optional<std::monostate> Result();
    void operator()() {}

    bool Become(CoroutineStatus status);
    ~Coroutine() = default;
private:
    void Destroy() override;
    void Resume() override;

    void ToExit();
private:
    std::shared_ptr<std::atomic<CoroutineStatus>> m_status;
    std::coroutine_handle<promise_type> m_handle;
    std::atomic<AwaiterBase*> m_awaiter = nullptr;
    std::shared_ptr<std::list<ExitHandle>> m_exit_cbs;
};




template<typename T, typename CoRtn>
class AsyncResult: public details::Awaiter<T>
{
public:
    AsyncResult(details::WaitAction* action, void* ctx);
    AsyncResult(T&& result);
    bool await_ready() const noexcept;
    //true will suspend, false will not
    bool await_suspend(std::coroutine_handle<typename Coroutine<CoRtn>::promise_type> handle) noexcept;
    T&& await_resume() const noexcept;
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
