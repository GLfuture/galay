#ifndef GALAY_COROUTINE_HPP
#define GALAY_COROUTINE_HPP

#include <any>
#include <memory>
#include <atomic>
#include <coroutine>
#include <variant>
#include <mutex>
#include <list>
#include <shared_mutex>
#include <string>
#include <functional>
#include <unordered_map>
#include "galay/common/Base.h"
#include "Log.h"

namespace galay {
    class Timer;
}

namespace galay {
    class Coroutine;
}

namespace galay::details
{
class CoroutineScheduler;

class WaitAction
{
public:
    using Coroutine_wptr = std::weak_ptr<Coroutine>;
    virtual ~WaitAction() = default;
    virtual bool HasEventToDo() = 0;
    virtual bool DoAction(Coroutine_wptr co, void* ctx) = 0;
};
}

namespace galay
{
class AwaiterBase
{
};

#define DEFAULT_COROUTINE_STORE_HASH_SIZE       2048


//只加入父协程，子协程的释放交给ContextCancel做
class CoroutineStore
{
public:
    CoroutineStore();
    void Reserve(size_t size);
    void AddCoroutine(std::weak_ptr<Coroutine> co);
    void RemoveCoroutine(std::weak_ptr<Coroutine> co);
    bool Exist(std::weak_ptr<Coroutine> co);
    void Clear();
private:
    std::shared_mutex m_mutex;
    std::unordered_map<Coroutine*, std::weak_ptr<Coroutine>> m_coroutines;
};

class Coroutine
{
public:
    Coroutine& operator=(Coroutine& other) = delete;
    Coroutine& operator=(const Coroutine& other) = delete;
public:
    using ptr = std::shared_ptr<Coroutine>;
    using wptr = std::weak_ptr<Coroutine>;
    class promise_type
    {
    public:
        static int get_return_object_on_alloaction_failure() noexcept { return -1; }
        Coroutine get_return_object() noexcept;
        static std::suspend_never initial_suspend() noexcept { return {}; }
        static std::suspend_always yield_value() noexcept { return {}; }
        static std::suspend_never final_suspend() noexcept { return {};  }
        static void unhandled_exception() noexcept {}
        void return_void () const noexcept {};
        Coroutine::wptr GetCoroutine() { return m_coroutine; }
        ~promise_type();
    private:
        Coroutine::ptr m_coroutine;
    };
    explicit Coroutine(std::coroutine_handle<promise_type> handle, details::CoroutineScheduler* scheduler) noexcept;
    Coroutine(Coroutine&& other) noexcept;
    Coroutine(const Coroutine& other) noexcept;
    Coroutine& operator=(Coroutine&& other) noexcept;
    
    std::coroutine_handle<promise_type> GetHandle() const { return this->m_handle; }
    details::CoroutineScheduler* BelongScheduler() const { return this->m_scheduler; }
    void Destroy() { m_handle.destroy(); }
    bool Done() const { return *m_is_done; }
    void Resume() const { return m_handle.resume(); }
    bool SetAwaiter(AwaiterBase* awaiter);
    AwaiterBase* GetAwaiter() const;

    void AppendExitCallback(const std::function<void()>& callback);
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
    bool await_ready() const noexcept;
    //true will suspend, false will not
    bool await_suspend(std::coroutine_handle<Coroutine::promise_type> handle) noexcept;
    T await_resume() const noexcept;
    void SetResult(T result);
    [[nodiscard]] Coroutine::wptr GetCoroutine() const;
private:
    void* m_ctx;
    T m_result;
    details::WaitAction* m_action;
    std::coroutine_handle<Coroutine::promise_type> m_handle;
};


//线程安全
class WaitGroup
{
public:
    WaitGroup(uint32_t count);
    void Done();
    void Reset(uint32_t count = 0);
    Awaiter<bool> Wait();
private:
    std::shared_mutex m_mutex;
    Coroutine::wptr m_coroutine;
    uint32_t m_count = 0;
};


}

namespace galay
{

extern CoroutineStore* GetCoroutineStore();

}

namespace galay::this_coroutine
{
    /*
        父协程一定需要加入到协程存储
    */
    extern Awaiter<void> AddToCoroutineStore();
    extern Awaiter<Coroutine::wptr> GetThisCoroutine();

    extern Awaiter<void> WaitAsyncExecute(const Coroutine& co);

    /*
        return false only when TimeScheduler is not running
        [ms] : ms
        [timer] : timer
        [scheduler] : coroutine_scheduler, this coroutine will resume at this scheduler
    */
    extern Awaiter<void> Sleepfor(int64_t ms);

    /*
        注意，直接传lambda会导致shared_ptr引用计数不增加，推荐使用bind,或者传lambda对象
        注意和AutoDestructor的回调区别，DeferExit的callback会在协程正常和非正常退出时调用
    */
    extern Awaiter<void> DeferExit(const std::function<void(void)>& callback);
    
    #define MIN_REMAIN_TIME     10
    /*用在father 协程中，通过father销毁child*/
    /*
        注意使用shared_ptr分配
        AutoDestructor的回调只会在超时时调用, 剩余时间小于MIN_REMAIN_TIME就会调用
    */
    class AutoDestructor: public std::enable_shared_from_this<AutoDestructor>
    {
    public:
        using ptr = std::shared_ptr<AutoDestructor>;
        AutoDestructor();
        Awaiter<void> Start(uint64_t ms);
        void SetCallback(const std::function<void(void)>& callback) { this->m_callback = callback; }
        bool Reflush();
        ~AutoDestructor() = default;
    private:
        std::shared_ptr<Timer> m_timer = nullptr;
        std::atomic_uint64_t m_last_active_time = 0;
        std::function<void(void)> m_callback;
    };
}

#include "Coroutine.tcc"

#endif
