#include "Awaiter.h"
#include "Log.h"

namespace galay::coroutine
{

Awaiter_void::Awaiter_void(details::WaitAction *action, void* ctx)
    : m_ctx(ctx), m_action(action), m_coroutine_handle(nullptr)
{

}

Awaiter_void::Awaiter_void()
    : m_ctx(nullptr), m_action(nullptr), m_coroutine_handle(nullptr)
{

}

bool Awaiter_void::await_ready() const noexcept
{
    return m_action == nullptr || !m_action->HasEventToDo();
}

bool Awaiter_void::await_suspend(const std::coroutine_handle<Coroutine::promise_type> handle) noexcept
{
    m_coroutine_handle = handle;
    Coroutine::wptr co = handle.promise().GetCoroutine();
    return m_action->DoAction(co, m_ctx);
}

void Awaiter_void::await_resume() const noexcept
{
}

void Awaiter_void::SetResult(const std::variant<std::monostate, int, bool, void*, std::string, GHandle> &result)
{
    if(! std::holds_alternative<std::monostate>(result) ) {
        throw std::runtime_error("Awaiter_void::SetResult: result is not monostate");
    }
}

Coroutine::wptr Awaiter_void::GetCoroutine() const
{
    if( m_coroutine_handle ) {
        return m_coroutine_handle.promise().GetCoroutine();
    }
    return {};
}

Awaiter_int::Awaiter_int(details::WaitAction* action, void* ctx)
    : m_ctx(ctx), m_result(0), m_action(action), m_coroutine_handle(nullptr)
{

}

Awaiter_int::Awaiter_int(const int result)
{
    m_action = nullptr;
    m_result = result;
    m_coroutine_handle = nullptr;
    m_ctx = nullptr;
}

bool Awaiter_int::await_ready() const noexcept
{
    return m_action == nullptr || !m_action->HasEventToDo();
}

bool Awaiter_int::await_suspend(const std::coroutine_handle<Coroutine::promise_type> handle) noexcept
{
    m_coroutine_handle = handle;
    coroutine::Coroutine::wptr co = handle.promise().GetCoroutine();
    if(co.expired()) {
        LogError("[Coroutine expired]");
        return false;
    }
    while(!co.lock()->SetAwaiter(this)){
        LogError("[Set awaiter failed]");
    }
    return m_action->DoAction(co, m_ctx);
}

int Awaiter_int::await_resume() const noexcept
{
    if( m_coroutine_handle ) {
        while(!m_coroutine_handle.promise().GetCoroutine().lock()->SetAwaiter(nullptr)){
            LogError("[Set awaiter failed]");
        }
    }
    return m_result;
}

void Awaiter_int::SetResult(const std::variant<std::monostate, int, bool, void*, std::string, GHandle> &result)
{
    if(! std::holds_alternative<int>(result)){
        throw std::runtime_error("Awaiter_int::SetResult: result is not int");
    }
    m_result = std::get<int>(result);
}

Coroutine::wptr Awaiter_int::GetCoroutine() const
{
    if( m_coroutine_handle ) {
        return m_coroutine_handle.promise().GetCoroutine();
    }
    return {};
}

Awaiter_bool::Awaiter_bool(details::WaitAction* action, void* ctx)
{
    m_action = action;
    m_result = false;
    m_coroutine_handle = nullptr;
    m_ctx = ctx;
}

Awaiter_bool::Awaiter_bool(const bool result)
{
    m_action = nullptr;
    m_result = result;
    m_coroutine_handle = nullptr;
    m_ctx = nullptr;
}

bool Awaiter_bool::await_ready() const noexcept
{
    return m_action == nullptr || !m_action->HasEventToDo();
}

bool Awaiter_bool::await_suspend(const std::coroutine_handle<Coroutine::promise_type> handle) noexcept
{
    m_coroutine_handle = handle;
    coroutine::Coroutine::wptr co = handle.promise().GetCoroutine();
    if(co.expired()) {
        LogError("[Coroutine expired]");
        return false;
    }
    while(!co.lock()->SetAwaiter(this)){
        LogError("[Set awaiter failed]");
    }
    return m_action->DoAction(co, m_ctx);
}

bool Awaiter_bool::await_resume() const noexcept
{
    if( m_coroutine_handle ) {
        while(!m_coroutine_handle.promise().GetCoroutine().lock()->SetAwaiter(nullptr)){
            LogError("[Set awaiter failed]");
        }
    }
    return m_result;
}

void Awaiter_bool::SetResult(const std::variant<std::monostate, int, bool, void*, std::string, GHandle> &result)
{
    if(! std::holds_alternative<bool>(result)){
        throw std::runtime_error("Awaiter_bool::SetResult: result is not bool");
    }
    m_result = std::get<bool>(result);
}

Coroutine::wptr Awaiter_bool::GetCoroutine() const
{
    if( m_coroutine_handle ) {
        return m_coroutine_handle.promise().GetCoroutine();
    }
    return {};
}

Awaiter_ptr::Awaiter_ptr(details::WaitAction *action, void* ctx)
{
    m_action = action;
    m_coroutine_handle = nullptr;
    m_ptr = nullptr;
    m_ctx = ctx;
}

Awaiter_ptr::Awaiter_ptr(void *ptr)
{
    m_action = nullptr;
    m_ptr = ptr;
    m_coroutine_handle = nullptr;
    m_ctx = nullptr;
}

bool Awaiter_ptr::await_ready() const noexcept
{
    return m_action == nullptr || !m_action->HasEventToDo();
}

bool Awaiter_ptr::await_suspend(const std::coroutine_handle<Coroutine::promise_type> handle) noexcept
{
    m_coroutine_handle = handle;
    coroutine::Coroutine::wptr co = handle.promise().GetCoroutine();
    if(co.expired()) {
        LogError("[Coroutine expired]");
        return false;
    }
    while(!co.lock()->SetAwaiter(this)){
        LogError("[Set awaiter failed]");
    }
    return m_action->DoAction(co, m_ctx);
}

void *Awaiter_ptr::await_resume() const noexcept
{
    if( m_coroutine_handle ) {
        while(!m_coroutine_handle.promise().GetCoroutine().lock()->SetAwaiter(nullptr)){
            LogError("[Set awaiter failed]");
        }
    }
    return m_ptr;
}

void Awaiter_ptr::SetResult(const std::variant<std::monostate, int, bool, void*, std::string, GHandle> &result)
{
    if(! std::holds_alternative<void*>(result)){
        throw std::runtime_error("Awaiter_ptr::SetResult: result is not ptr");
    }
    m_ptr = std::get<void*>(result);
}

Coroutine::wptr Awaiter_ptr::GetCoroutine() const
{
    if( m_coroutine_handle ) {
        return m_coroutine_handle.promise().GetCoroutine();
    }
    return {};
}

Awaiter_string::Awaiter_string(details::WaitAction *action, void* ctx)
{
    m_action = action;
    m_result = "";
    m_ctx = ctx;
    m_coroutine_handle = nullptr;
}

Awaiter_string::Awaiter_string(const std::string& result)
{
    m_action = nullptr;
    m_result = result;
    m_coroutine_handle = nullptr;
    m_ctx = nullptr;
}

bool Awaiter_string::await_ready() const noexcept
{
    return m_action == nullptr || !m_action->HasEventToDo();
}
bool Awaiter_string::await_suspend(std::coroutine_handle<Coroutine::promise_type> handle) noexcept
{
    m_coroutine_handle = handle;
    coroutine::Coroutine::wptr co = handle.promise().GetCoroutine();
    if(co.expired()) {
        LogError("[Coroutine expired]");
        return false;
    }
    while(!co.lock()->SetAwaiter(this)){
        LogError("[Set awaiter failed]");
    }
    return m_action->DoAction(co, m_ctx);
}

std::string Awaiter_string::await_resume() const noexcept
{
    if( m_coroutine_handle ) {
        while(!m_coroutine_handle.promise().GetCoroutine().lock()->SetAwaiter(nullptr)){
            LogError("[Set awaiter failed]");
        }
    }
    return m_result;
}


void Awaiter_string::SetResult(const std::variant<std::monostate, int, bool, void*, std::string, GHandle> &result)
{
    if(! std::holds_alternative<std::string>(result)){
        throw std::runtime_error("Awaiter_string::SetResult: result is not string");
    }
    m_result = std::get<std::string>(result);
}

Coroutine::wptr Awaiter_string::GetCoroutine() const
{
    if( m_coroutine_handle ) {
        return m_coroutine_handle.promise().GetCoroutine();
    }
    return {};
}



//GHandle

Awaiter_GHandle::Awaiter_GHandle(details::WaitAction *action, void* ctx)
    : m_ctx(ctx), m_result({-1}), m_action(action), m_coroutine_handle(nullptr)
{

}

Awaiter_GHandle::Awaiter_GHandle(GHandle handle)
    : m_ctx(nullptr), m_result(handle), m_action(nullptr), m_coroutine_handle(nullptr)
{

}

bool Awaiter_GHandle::await_ready() const noexcept
{
    return m_action == nullptr || !m_action->HasEventToDo();
}
bool Awaiter_GHandle::await_suspend(std::coroutine_handle<Coroutine::promise_type> handle) noexcept
{
    m_coroutine_handle = handle;
    coroutine::Coroutine::wptr co = handle.promise().GetCoroutine();
    if(co.expired()) {
        LogError("[Coroutine expired]");
        return false;
    }
    while(!co.lock()->SetAwaiter(this)){
        LogError("[Set awaiter failed]");
    }
    return m_action->DoAction(co, m_ctx);
}

GHandle Awaiter_GHandle::await_resume() const noexcept
{
    if( m_coroutine_handle ) {
        while(!m_coroutine_handle.promise().GetCoroutine().lock()->SetAwaiter(nullptr)){
            LogError("[Set awaiter failed]");
        }
    }
    return m_result;
}


void Awaiter_GHandle::SetResult(const std::variant<std::monostate, int, bool, void*, std::string, GHandle> &result)
{
    if(! std::holds_alternative<GHandle>(result)){
        throw std::runtime_error("Awaiter_string::SetResult: result is not string");
    }
    m_result = std::get<GHandle>(result);
}

Coroutine::wptr Awaiter_GHandle::GetCoroutine() const
{
    if( m_coroutine_handle ) {
        return m_coroutine_handle.promise().GetCoroutine();
    }
    return {};
}



}