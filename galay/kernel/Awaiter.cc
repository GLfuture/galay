#include "Awaiter.h"
#include <spdlog/spdlog.h>

namespace galay::coroutine
{
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
    return this->m_action->DoAction(handle.promise().GetCoroutine());
}

int Awaiter_int::await_resume() const noexcept
{
    if( ! this->m_coroutine_handle ) {
        return m_result;
    }
    return std::any_cast<int>(this->m_coroutine_handle.promise().GetCoroutine()->GetContext());
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
    return m_action->DoAction(handle.promise().GetCoroutine());
}

bool Awaiter_bool::await_resume() const noexcept
{
    if( ! this->m_coroutine_handle ) {
        return m_result;
    }
    return std::any_cast<bool>(this->m_coroutine_handle.promise().GetCoroutine()->GetContext());
}

Coroutine* Awaiter_bool::GetCoroutine()
{
    if( m_coroutine_handle ) {
        return m_coroutine_handle.promise().GetCoroutine();
    }
    return nullptr;
}

}