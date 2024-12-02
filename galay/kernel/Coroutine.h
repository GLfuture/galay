#ifndef GALAY_COROUTINE_H
#define GALAY_COROUTINE_H

#include <any>
#include <memory>
#include <atomic>
#include <coroutine>
#include <variant>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <functional>
#include <unordered_set>
#include "galay/common/Base.h"

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
    virtual ~Awaiter() = default;
    virtual void SetResult(const std::variant<std::monostate, int, bool, void*, std::string, GHandle>& result) = 0;
};

#define DEFAULT_COROUTINE_STORE_HASH_SIZE       2048

class CoroutineStore
{
public:
    CoroutineStore();
    void Reserve(size_t size);
    void AddCoroutine(Coroutine* co);
    void RemoveCoroutine(Coroutine* co);
    bool Exist(Coroutine* co);
    void Clear();
private:
    std::shared_mutex m_mutex;
    std::unordered_set<Coroutine*> m_coroutines;
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
        static int get_return_object_on_alloaction_failure() noexcept { return -1; }
        Coroutine get_return_object() noexcept;
        static std::suspend_never initial_suspend() noexcept { return {}; }
        static std::suspend_always yield_value() noexcept { return {}; }
        static std::suspend_never final_suspend() noexcept { return {};  }
        static void unhandled_exception() noexcept {}
        void return_void () const noexcept {};
        [[nodiscard]] Coroutine* GetCoroutine() const { return m_coroutine; }
        [[nodiscard]] CoroutineStore* GetStore() const { return m_store; }
        ~promise_type();
    private:
        CoroutineStore* m_store = nullptr ;
        Coroutine* m_coroutine = nullptr;
    };
    explicit Coroutine(std::coroutine_handle<promise_type> handle) noexcept;
    Coroutine(Coroutine&& other) noexcept;
    Coroutine(const Coroutine& other) noexcept;
    Coroutine& operator=(Coroutine&& other) noexcept;
    
    std::coroutine_handle<promise_type> GetHandle() { return this->m_handle; }
    void Destroy();
    bool Done() const { return m_handle.done(); }
    void Resume() const { return m_handle.resume(); }
    bool SetAwaiter(Awaiter* awaiter);
    void SetExitCallback(const std::function<void()>& callback);
    Awaiter* GetAwaiter() const;
    ~Coroutine() = default;
private:
    void Exit();
private:
    std::atomic<Awaiter*> m_awaiter = nullptr;
    std::coroutine_handle<promise_type> m_handle;
    std::function<void()> m_exit_cb;
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
    extern coroutine::Awaiter_void Exit(const std::function<void(void)>& callback);
}

#endif
