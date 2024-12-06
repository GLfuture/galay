#ifndef GALAY_AWAIT_H
#define GALAY_AWAIT_H

#include "Coroutine.h"
#include <string>

namespace galay::details
{
class WaitEvent;

class WaitAction
{
public:
    virtual ~WaitAction() = default;
    virtual bool HasEventToDo() = 0;
    virtual bool DoAction(coroutine::Coroutine* co, void* ctx) = 0;
};
}

namespace galay::coroutine
{

class Awaiter_void final : public Awaiter
{
public:
    Awaiter_void(details::WaitAction* action, void* ctx);
    Awaiter_void();
    [[nodiscard]] bool await_ready() const noexcept;
    //true will suspend, false will not
    bool await_suspend(std::coroutine_handle<Coroutine::promise_type> handle) noexcept;
    void await_resume() const noexcept;
    void SetResult(const std::variant<std::monostate, int, bool, void*, std::string, GHandle>& result) override;
    [[nodiscard]] Coroutine* GetCoroutine() const;
private:
    void* m_ctx;
    details::WaitAction* m_action;
    std::coroutine_handle<Coroutine::promise_type> m_coroutine_handle;
};

class Awaiter_int final : public Awaiter
{
public:
    Awaiter_int(details::WaitAction* action, void* ctx);
    explicit Awaiter_int(int result);
    [[nodiscard]] bool await_ready() const noexcept;
    //true will suspend, false will not
    bool await_suspend(std::coroutine_handle<Coroutine::promise_type> handle) noexcept;
    [[nodiscard]] int await_resume() const noexcept;
    void SetResult(const std::variant<std::monostate, int, bool, void*, std::string, GHandle>& result) override;
    [[nodiscard]] Coroutine* GetCoroutine() const;
private:
    void* m_ctx;
    int m_result;
    details::WaitAction* m_action;
    std::coroutine_handle<Coroutine::promise_type> m_coroutine_handle;
};

class Awaiter_bool final : public Awaiter
{
public:
    Awaiter_bool(details::WaitAction* action, void* ctx);
    explicit Awaiter_bool(bool result);
    [[nodiscard]] bool await_ready() const noexcept;
    //true will suspend, false will not
    bool await_suspend(std::coroutine_handle<Coroutine::promise_type> handle) noexcept;
    [[nodiscard]] bool await_resume() const noexcept;
    void SetResult(const std::variant<std::monostate, int, bool, void*, std::string, GHandle>& result) override;
    [[nodiscard]] Coroutine* GetCoroutine() const;
private:
    void* m_ctx;
    bool m_result;
    details::WaitAction* m_action;
    std::coroutine_handle<Coroutine::promise_type> m_coroutine_handle;
};

class Awaiter_ptr final : public Awaiter
{
public:
    Awaiter_ptr(details::WaitAction* action, void* ctx);
    explicit Awaiter_ptr(void* ptr);
    [[nodiscard]] bool await_ready() const noexcept;
    //true will suspend, false will not
    bool await_suspend(std::coroutine_handle<Coroutine::promise_type> handle) noexcept;
    [[nodiscard]] void* await_resume() const noexcept;
    void SetResult(const std::variant<std::monostate, int, bool, void*, std::string, GHandle>& result) override;
    [[nodiscard]] Coroutine* GetCoroutine() const;
private:
    void* m_ctx;
    void* m_ptr;
    details::WaitAction* m_action;
    std::coroutine_handle<Coroutine::promise_type> m_coroutine_handle;
};


class Awaiter_string final : public Awaiter
{
public:
    Awaiter_string(details::WaitAction* action, void* ctx);
    explicit Awaiter_string(const std::string& result);
    [[nodiscard]] bool await_ready() const noexcept;
    //true will suspend, false will not
    bool await_suspend(std::coroutine_handle<Coroutine::promise_type> handle) noexcept;
    [[nodiscard]] std::string await_resume() const noexcept;
    void SetResult(const std::variant<std::monostate, int, bool, void*, std::string, GHandle>& result) override;
    [[nodiscard]] Coroutine* GetCoroutine() const;
private:
    void* m_ctx;
    std::string m_result;
    details::WaitAction* m_action;
    std::coroutine_handle<Coroutine::promise_type> m_coroutine_handle;
};

class Awaiter_GHandle final : public Awaiter
{
public:
    Awaiter_GHandle(details::WaitAction* action, void* ctx);
    explicit Awaiter_GHandle(GHandle handle);
    [[nodiscard]] bool await_ready() const noexcept;
    //true will suspend, false will not
    bool await_suspend(std::coroutine_handle<Coroutine::promise_type> handle) noexcept;
    [[nodiscard]] GHandle await_resume() const noexcept;
    void SetResult(const std::variant<std::monostate, int, bool, void*, std::string, GHandle>& result) override;
    [[nodiscard]] Coroutine* GetCoroutine() const;
private:
    void* m_ctx;
    GHandle m_result;
    details::WaitAction* m_action;
    std::coroutine_handle<Coroutine::promise_type> m_coroutine_handle;
};

}

#endif
