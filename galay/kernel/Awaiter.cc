#include "Awaiter.h"
#include <spdlog/spdlog.h>

namespace galay::coroutine
{

Awaiter_void::Awaiter_void(action::WaitAction *action)
{
    this->m_action = action;
    this->m_coroutine_handle = nullptr;
}

Awaiter_void::Awaiter_void()
{
    this->m_action = nullptr;
    this->m_coroutine_handle = nullptr;
}

bool Awaiter_void::await_ready() const noexcept
{
    return this->m_action == nullptr || !this->m_action->HasEventToDo();
}

bool Awaiter_void::await_suspend(std::coroutine_handle<Coroutine::promise_type> handle) noexcept
{
    this->m_coroutine_handle = handle;
    coroutine::Coroutine* co = handle.promise().GetCoroutine();
    co->SetAwaiter(this);
    return this->m_action->DoAction(co);
}

void Awaiter_void::await_resume() const noexcept
{
    if( m_coroutine_handle ) {
        coroutine::Coroutine* co = m_coroutine_handle.promise().GetCoroutine();
        co->SetAwaiter(nullptr);
    }
}

void Awaiter_void::SetResult(const std::variant<std::monostate, int, bool, void*, std::string, GHandle> &result)
{
    if(! std::holds_alternative<std::monostate>(result) ) {
        spdlog::error("Awaiter_void::SetResult: result is not monostate");
        return;
    }
}

Coroutine *Awaiter_void::GetCoroutine()
{
    if( m_coroutine_handle ) {
        return m_coroutine_handle.promise().GetCoroutine();
    }
    return nullptr;
}

Awaiter_int::Awaiter_int(action::WaitAction* action)
{
    this->m_action = action;
    this->m_result = 0;
    this->m_coroutine_handle = nullptr;
}

Awaiter_int::Awaiter_int(int result)
{
    this->m_action = nullptr;
    this->m_result = result;
    this->m_coroutine_handle = nullptr;
}

bool Awaiter_int::await_ready() const noexcept
{
    return this->m_action == nullptr || !this->m_action->HasEventToDo();
}

bool Awaiter_int::await_suspend(std::coroutine_handle<Coroutine::promise_type> handle) noexcept
{
    this->m_coroutine_handle = handle;
    coroutine::Coroutine* co = handle.promise().GetCoroutine();
    co->SetAwaiter(this);
    spdlog::debug("Awaiter_int::await_suspend");
    return this->m_action->DoAction(co);
}

int Awaiter_int::await_resume() const noexcept
{
    if( m_coroutine_handle ) {
        coroutine::Coroutine* co = m_coroutine_handle.promise().GetCoroutine();
        co->SetAwaiter(nullptr);
    }
    return m_result;
}

void Awaiter_int::SetResult(const std::variant<std::monostate, int, bool, void*, std::string, GHandle> &result)
{
    if(! std::holds_alternative<int>(result)){
        spdlog::error("Awaiter_int::SetResult: result is not int");
        return;
    }
    m_result = std::get<int>(result);
}

Coroutine *Awaiter_int::GetCoroutine()
{
    if( m_coroutine_handle ) {
        return m_coroutine_handle.promise().GetCoroutine();
    }
    return nullptr;
}

Awaiter_bool::Awaiter_bool(action::WaitAction* action)
{
    this->m_action = action;
    this->m_result = false;
    this->m_coroutine_handle = nullptr;
}

Awaiter_bool::Awaiter_bool(bool result)
{
    this->m_action = nullptr;
    this->m_result = result;
    this->m_coroutine_handle = nullptr;
}

bool Awaiter_bool::await_ready() const noexcept
{
    return this->m_action == nullptr || !this->m_action->HasEventToDo();
}

bool Awaiter_bool::await_suspend(std::coroutine_handle<Coroutine::promise_type> handle) noexcept
{
    this->m_coroutine_handle = handle;
    coroutine::Coroutine* co = handle.promise().GetCoroutine();
    co->SetAwaiter(this);
    spdlog::debug("Awaiter_bool::await_suspend");
    return m_action->DoAction(co);
}

bool Awaiter_bool::await_resume() const noexcept
{
    if( m_coroutine_handle ) {
        coroutine::Coroutine* co = m_coroutine_handle.promise().GetCoroutine();
        co->SetAwaiter(nullptr);
    }
    return m_result;
}

void Awaiter_bool::SetResult(const std::variant<std::monostate, int, bool, void*, std::string, GHandle> &result)
{
    if(! std::holds_alternative<bool>(result)){
        spdlog::error("Awaiter_bool::SetResult: result is not bool");
        return;
    }
    m_result = std::get<bool>(result);
}

Coroutine* Awaiter_bool::GetCoroutine()
{
    if( m_coroutine_handle ) {
        return m_coroutine_handle.promise().GetCoroutine();
    }
    return nullptr;
}

Awaiter_ptr::Awaiter_ptr(action::WaitAction *action)
{
    this->m_action = action;
    this->m_coroutine_handle = nullptr;
}

Awaiter_ptr::Awaiter_ptr(void *ptr)
{
    this->m_action = nullptr;
    this->m_ptr = ptr;
    this->m_coroutine_handle = nullptr;
}

bool Awaiter_ptr::await_ready() const noexcept
{
    return this->m_action == nullptr || !this->m_action->HasEventToDo();
}

bool Awaiter_ptr::await_suspend(std::coroutine_handle<Coroutine::promise_type> handle) noexcept
{
    this->m_coroutine_handle = handle;
    coroutine::Coroutine* co = handle.promise().GetCoroutine();
    co->SetAwaiter(this);
    return this->m_action->DoAction(co);
}

void *Awaiter_ptr::await_resume() const noexcept
{
    if( m_coroutine_handle ) {
        coroutine::Coroutine* co = m_coroutine_handle.promise().GetCoroutine();
        co->SetAwaiter(nullptr);
    }
    return m_ptr;
}

void Awaiter_ptr::SetResult(const std::variant<std::monostate, int, bool, void*, std::string, GHandle> &result)
{
    if(! std::holds_alternative<void*>(result)){
        spdlog::error("Awaiter_ptr::SetResult: result is not ptr");
        return;
    }
    m_ptr = std::get<void*>(result);
}

Coroutine *Awaiter_ptr::GetCoroutine()
{
    if( m_coroutine_handle ) {
        return m_coroutine_handle.promise().GetCoroutine();
    }
    return nullptr;
}

Awaiter_string::Awaiter_string(action::WaitAction *action)
{
    this->m_action = action;
    this->m_coroutine_handle = nullptr;
}

Awaiter_string::Awaiter_string(std::string result)
{
    this->m_action = nullptr;
    this->m_result = result;
    this->m_coroutine_handle = nullptr;
}

bool Awaiter_string::await_ready() const noexcept
{
    return this->m_action == nullptr || !this->m_action->HasEventToDo();
}
bool Awaiter_string::await_suspend(std::coroutine_handle<Coroutine::promise_type> handle) noexcept
{
    this->m_coroutine_handle = handle;
    coroutine::Coroutine* co = handle.promise().GetCoroutine();
    co->SetAwaiter(this);
    return this->m_action->DoAction(co);
}

std::string Awaiter_string::await_resume() const noexcept
{
    if( m_coroutine_handle ) {
        coroutine::Coroutine* co = m_coroutine_handle.promise().GetCoroutine();
        co->SetAwaiter(nullptr);
    }
    return m_result;
}


void Awaiter_string::SetResult(const std::variant<std::monostate, int, bool, void*, std::string, GHandle> &result)
{
    if(! std::holds_alternative<std::string>(result)){
        spdlog::error("Awaiter_string::SetResult: result is not string");
        return;
    }
    m_result = std::get<std::string>(result);
}

Coroutine *Awaiter_string::GetCoroutine()
{
    if( m_coroutine_handle ) {
        return m_coroutine_handle.promise().GetCoroutine();
    }
    return nullptr;
}



//GHandle

Awaiter_GHandle::Awaiter_GHandle(action::WaitAction *action)
{
    this->m_action = action;
    this->m_coroutine_handle = nullptr;
    this->m_result.fd = -1;
}

Awaiter_GHandle::Awaiter_GHandle(GHandle result)
{
    this->m_action = nullptr;
    this->m_result = result;
    this->m_coroutine_handle = nullptr;
}

bool Awaiter_GHandle::await_ready() const noexcept
{
    return this->m_action == nullptr || !this->m_action->HasEventToDo();
}
bool Awaiter_GHandle::await_suspend(std::coroutine_handle<Coroutine::promise_type> handle) noexcept
{
    this->m_coroutine_handle = handle;
    coroutine::Coroutine* co = handle.promise().GetCoroutine();
    co->SetAwaiter(this);
    return this->m_action->DoAction(co);
}

GHandle Awaiter_GHandle::await_resume() const noexcept
{
    if( m_coroutine_handle ) {
        coroutine::Coroutine* co = m_coroutine_handle.promise().GetCoroutine();
        co->SetAwaiter(nullptr);
    }
    return m_result;
}


void Awaiter_GHandle::SetResult(const std::variant<std::monostate, int, bool, void*, std::string, GHandle> &result)
{
    if(! std::holds_alternative<GHandle>(result)){
        spdlog::error("Awaiter_string::SetResult: result is not string");
        return;
    }
    m_result = std::get<GHandle>(result);
}

Coroutine *Awaiter_GHandle::GetCoroutine()
{
    if( m_coroutine_handle ) {
        return m_coroutine_handle.promise().GetCoroutine();
    }
    return nullptr;
}



}