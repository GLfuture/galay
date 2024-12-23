#ifndef GALAY_COROUTINE_TCC
#define GALAY_COROUTINE_TCC

#include "Coroutine.hpp"

namespace galay
{

template<typename T>
inline Awaiter<T>::Awaiter(details::WaitAction* action, void* ctx) 
    : m_ctx(ctx), m_action(action), m_handle(nullptr) 
{
}

template<typename T>
inline Awaiter<T>::Awaiter(T result) 
    : m_result(result), m_ctx(nullptr), m_action(nullptr), m_handle(nullptr)
{
}

template<typename T>
inline bool Awaiter<T>::await_ready() const noexcept 
{
    return m_action == nullptr || !m_action->HasEventToDo();
}

template<typename T>
inline bool Awaiter<T>::await_suspend(std::coroutine_handle<Coroutine::promise_type> handle) noexcept 
{
    m_handle = handle;
    Coroutine::wptr co = handle.promise().GetCoroutine();
    if(co.expired()) {
        LogError("[Coroutine expired]");
        return false;
    }
    while(!co.lock()->SetAwaiter(this)){
        LogError("[Set awaiter failed]");
    }
    return m_action->DoAction(co, m_ctx);
}

template<typename T>
inline T Awaiter<T>::await_resume() const noexcept 
{
    if( m_handle ) {
        while(!m_handle.promise().GetCoroutine().lock()->SetAwaiter(nullptr)){
            LogError("[Set awaiter failed]");
        }
    }
    return m_result;
}

template<typename T>
inline void Awaiter<T>::SetResult(T result) 
{
    m_result = result;
}

template<typename T>
inline Coroutine::wptr Awaiter<T>::GetCoroutine() const 
{
    if(m_handle) return m_handle.promise().GetCoroutine();
    return {};
}

template<>
class Awaiter<void>: public AwaiterBase
{
public:
    Awaiter(details::WaitAction* action, void* ctx) 
        : m_ctx(ctx), m_action(action), m_handle(nullptr) {}
    Awaiter() 
        : m_ctx(nullptr), m_action(nullptr), m_handle(nullptr){}
    bool await_ready() const noexcept {
        return m_action == nullptr || !m_action->HasEventToDo();
    }
    //true will suspend, false will not
    bool await_suspend(std::coroutine_handle<Coroutine::promise_type> handle) noexcept {
        m_handle = handle;
        Coroutine::wptr co = handle.promise().GetCoroutine();
        if(co.expired()) {
            LogError("[Coroutine expired]");
            return false;
        }
        while(!co.lock()->SetAwaiter(this)){
            LogError("[Set awaiter failed]");
        }
        return m_action->DoAction(co, m_ctx);
    }
    void await_resume() const noexcept {
        if( m_handle ) {
            while(!m_handle.promise().GetCoroutine().lock()->SetAwaiter(nullptr)){
                LogError("[Set awaiter failed]");
            }
        }
    }

    [[nodiscard]] Coroutine::wptr GetCoroutine() const {
        if( m_handle ) {
            return m_handle.promise().GetCoroutine();
        }
        return {};
    }
private:
    void* m_ctx;
    details::WaitAction* m_action;
    std::coroutine_handle<Coroutine::promise_type> m_handle;
};



}


#endif