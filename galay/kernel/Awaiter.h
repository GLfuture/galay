#ifndef GALAY_AWAIT_H
#define GALAY_AWAIT_H

#include "Coroutine.h"
#include <string>

namespace galay::event
{
    class WaitEvent;
}

namespace galay::action
{

class WaitAction
{
public:
    virtual bool HasEventToDo() = 0;
    virtual bool DoAction(coroutine::Coroutine* co) = 0;
};
}

namespace galay::coroutine
{

class Awaiter_void: public Awaiter
{
public:
    Awaiter_void(action::WaitAction* action);
    Awaiter_void();
    bool await_ready() const noexcept;
    //true will suspend, false will not
    bool await_suspend(std::coroutine_handle<Coroutine::promise_type> handle) noexcept;
    void await_resume() const noexcept;
    virtual void SetResult(const std::variant<std::monostate, int, bool, void*, std::string, GHandle>& result) override;
    Coroutine* GetCoroutine();
private:
    action::WaitAction* m_action;
    std::coroutine_handle<Coroutine::promise_type> m_coroutine_handle;
};

class Awaiter_int: public Awaiter
{
public:
    Awaiter_int(action::WaitAction* action);
    Awaiter_int(int result);
    bool await_ready() const noexcept;
    //true will suspend, false will not
    bool await_suspend(std::coroutine_handle<Coroutine::promise_type> handle) noexcept;
    int await_resume() const noexcept;
    virtual void SetResult(const std::variant<std::monostate, int, bool, void*, std::string, GHandle>& result) override;
    Coroutine* GetCoroutine();
private:
    int m_result;
    action::WaitAction* m_action;
    std::coroutine_handle<Coroutine::promise_type> m_coroutine_handle;
};

class Awaiter_bool: public Awaiter
{
public:
    Awaiter_bool(action::WaitAction* action);
    Awaiter_bool(bool result);
    bool await_ready() const noexcept;
    //true will suspend, false will not
    bool await_suspend(std::coroutine_handle<Coroutine::promise_type> handle) noexcept;
    bool await_resume() const noexcept;
    virtual void SetResult(const std::variant<std::monostate, int, bool, void*, std::string, GHandle>& result) override;
    Coroutine* GetCoroutine();
private:
    bool m_result;
    action::WaitAction* m_action;
    std::coroutine_handle<Coroutine::promise_type> m_coroutine_handle;
};

class Awaiter_ptr: public Awaiter
{
public:
    Awaiter_ptr(action::WaitAction* action);
    Awaiter_ptr(void* ptr);
    bool await_ready() const noexcept;
    //true will suspend, false will not
    bool await_suspend(std::coroutine_handle<Coroutine::promise_type> handle) noexcept;
    void* await_resume() const noexcept;
    virtual void SetResult(const std::variant<std::monostate, int, bool, void*, std::string, GHandle>& result) override;
    Coroutine* GetCoroutine();
private:
    void* m_ptr;
    action::WaitAction* m_action;
    std::coroutine_handle<Coroutine::promise_type> m_coroutine_handle;
};


class Awaiter_string: public Awaiter
{
public:
    Awaiter_string(action::WaitAction* action);
    Awaiter_string(std::string result);
    bool await_ready() const noexcept;
    //true will suspend, false will not
    bool await_suspend(std::coroutine_handle<Coroutine::promise_type> handle) noexcept;
    std::string await_resume() const noexcept;
    virtual void SetResult(const std::variant<std::monostate, int, bool, void*, std::string, GHandle>& result) override;
    Coroutine* GetCoroutine();
private:
    std::string m_result;
    action::WaitAction* m_action;
    std::coroutine_handle<Coroutine::promise_type> m_coroutine_handle;
};

class Awaiter_GHandle: public Awaiter
{
public:
    Awaiter_GHandle(action::WaitAction* action);
    Awaiter_GHandle(GHandle handle);
    bool await_ready() const noexcept;
    //true will suspend, false will not
    bool await_suspend(std::coroutine_handle<Coroutine::promise_type> handle) noexcept;
    GHandle await_resume() const noexcept;
    virtual void SetResult(const std::variant<std::monostate, int, bool, void*, std::string, GHandle>& result) override;
    Coroutine* GetCoroutine();
private:
    GHandle m_result;
    action::WaitAction* m_action;
    std::coroutine_handle<Coroutine::promise_type> m_coroutine_handle;
};

}

#endif
