#ifndef GALAY_COROUTINE_TCC
#define GALAY_COROUTINE_TCC

#include "Coroutine.hpp"

namespace galay::details
{



template <typename T>
inline Awaiter<T>::Awaiter(details::WaitAction *action, void *ctx)
    : m_ctx(ctx), m_action(action)
{
}

template <typename T>
inline Awaiter<T>::Awaiter(T result)
    : m_result(result), m_action(nullptr), m_ctx(nullptr)
{
}

template <typename T>
inline void Awaiter<T>::SetResult(T result)
{
    m_result = result;
}

}

namespace galay
{

template <typename T>
template <typename...Args>
inline PromiseType<T>::PromiseType(RoutineCtx::ptr ctx, Args&&...args)
    : m_ctx(ctx)
{
    if(!m_ctx) {
        panic("ctx is nullptr");
    }
}

template <typename T>
inline Coroutine<T> PromiseType<T>::get_return_object() noexcept
{
    m_coroutine = std::make_shared<Coroutine<T>>(std::coroutine_handle<PromiseType>::from_promise(*this));
    m_ctx->m_co = m_coroutine;
    return *m_coroutine;
}

template<typename T>
inline std::suspend_always PromiseType<T>::yield_value(T&& value) noexcept 
{ 
    *(m_coroutine->m_result) = std::move(value); return {}; 
}

template<typename T>
inline void PromiseType<T>::return_value(T&& value) const noexcept 
{ 
    *(m_coroutine->m_result) = std::move(value); 
}


template<typename T>
inline PromiseType<T>::~PromiseType() 
{
    //防止协程运行时resume创建出新的协程，析构时重复析构
    bool origin = false;
    if(!m_coroutine->m_is_done->compare_exchange_strong(origin, true)) {
        return;
    }
    if(m_coroutine) {
        m_coroutine->ToExit();
    }
}

template<typename ...Args>
inline PromiseType<void>::PromiseType(RoutineCtx::ptr ctx, Args&&... agrs)
    : m_ctx(ctx)
{
    if(!m_ctx) {
        panic("ctx is nullptr");
    }
}

inline Coroutine<void> PromiseType<void>::get_return_object() noexcept
{
    m_coroutine = std::make_shared<Coroutine<void>>(std::coroutine_handle<PromiseType<void>>::from_promise(*this));
    m_ctx->m_co = m_coroutine;
    return *m_coroutine;
}

inline PromiseType<void>::~PromiseType()
{
    bool origin = false;
    if(!m_coroutine->m_is_done->compare_exchange_strong(origin, true)) {
        return;
    }
    if(m_coroutine) {
        m_coroutine->ToExit();
    }
}


template<typename T>
inline Coroutine<T>::Coroutine(const std::coroutine_handle<promise_type> handle) noexcept
{
    m_is_done = std::make_shared<std::atomic_bool>(false);
    m_result = std::make_shared<std::optional<T>>();
    m_handle = handle;
    m_awaiter = nullptr;
    m_exit_cbs = std::make_shared<std::list<std::function<void()>>>();
}

template<typename T>
inline Coroutine<T>::Coroutine(Coroutine&& other) noexcept
{
    m_handle = other.m_handle;
    other.m_handle = nullptr;
    m_awaiter.store(other.m_awaiter);
    other.m_awaiter = nullptr;
    m_is_done = other.m_is_done;
    other.m_is_done.reset();
    m_result = other.m_result;
    other.m_result.reset();
    m_exit_cbs = other.m_exit_cbs;
    other.m_exit_cbs.reset();
}

template<typename T>
inline Coroutine<T>::Coroutine(const Coroutine& other) noexcept
{
    m_awaiter.store(other.m_awaiter);
    m_handle = other.m_handle;
    m_exit_cbs = other.m_exit_cbs;
    m_is_done = other.m_is_done;
    m_result = other.m_result;
}

template<typename T>
inline Coroutine<T>& Coroutine<T>::operator=(Coroutine&& other) noexcept
{
    m_handle = other.m_handle;
    other.m_handle = nullptr;
    m_awaiter.store(other.m_awaiter);
    other.m_awaiter = nullptr;
    m_is_done = other.m_is_done;
    other.m_is_done.reset();
    m_result = other.m_result;
    other.m_result.reset();
    m_exit_cbs = other.m_exit_cbs;
    other.m_exit_cbs.reset();
    return *this;
}

template<typename T>
Coroutine<T>& Coroutine<T>::operator=(const Coroutine& other) noexcept
{
    m_awaiter.store(other.m_awaiter);
    m_is_done = other.m_is_done;
    m_handle = other.m_handle;
    m_exit_cbs = other.m_exit_cbs;
    m_result = other.m_result;
    return *this;
}

template<typename T>
inline details::CoroutineScheduler* Coroutine<T>::BelongScheduler() const
{
    if(m_is_done->load()) {
        return nullptr;
    }
    return m_handle.promise().m_ctx->GetScheduler();
}

template<typename T>
inline bool Coroutine<T>::SetAwaiter(AwaiterBase* awaiter)
{
    if(AwaiterBase* t = m_awaiter.load(); !m_awaiter.compare_exchange_strong(t, awaiter)) {
        return false;
    }
    return true;
}

template<typename T>
inline void Coroutine<T>::AppendExitCallback(const std::function<void()>& callback)
{
    m_exit_cbs->emplace_front(callback);
}

template<typename T>
inline AwaiterBase* Coroutine<T>::GetAwaiter() const
{
    return m_awaiter.load();
}

template<typename T>
inline void Coroutine<T>::ToExit()
{
    while(!m_exit_cbs->empty()) {
        m_exit_cbs->front()();
        m_exit_cbs->pop_front();
    }
}

inline Coroutine<void>::Coroutine(const std::coroutine_handle<promise_type> handle) noexcept
{
    m_is_done = std::make_shared<std::atomic_bool>(false);
    m_exit_cbs = std::make_shared<std::list<std::function<void()>>>();
    m_handle = handle;
    m_awaiter = nullptr;
}

inline Coroutine<void>::Coroutine(Coroutine&& other) noexcept
{
    m_handle = other.m_handle;
    other.m_handle = nullptr;
    m_awaiter.store(other.m_awaiter);
    other.m_awaiter = nullptr;
    m_exit_cbs = other.m_exit_cbs;
    other.m_exit_cbs.reset();
    m_is_done = other.m_is_done;
    other.m_is_done.reset();
}

inline Coroutine<void>::Coroutine(const Coroutine& other) noexcept
{
    m_awaiter.store(other.m_awaiter);
    m_handle = other.m_handle;
    m_exit_cbs = other.m_exit_cbs;
    m_is_done = other.m_is_done;
}

inline Coroutine<void>& Coroutine<void>::operator=(Coroutine&& other) noexcept
{
    m_handle = other.m_handle;
    other.m_handle = nullptr;
    m_awaiter.store(other.m_awaiter);
    other.m_awaiter = nullptr;
    m_is_done = other.m_is_done;
    other.m_is_done.reset();
    m_exit_cbs = other.m_exit_cbs;
    other.m_exit_cbs.reset();
    return *this;
}

inline Coroutine<void>& Coroutine<void>::operator=(const Coroutine &other) noexcept
{
    m_awaiter.store(other.m_awaiter);
    m_handle = other.m_handle;
    m_exit_cbs = other.m_exit_cbs;
    m_is_done = other.m_is_done;
    return *this;
}

inline details::CoroutineScheduler* Coroutine<void>::BelongScheduler() const
{
    if(m_is_done->load()) {
        return nullptr;
    }
    return m_handle.promise().m_ctx->GetScheduler();
}

inline bool Coroutine<void>::SetAwaiter(AwaiterBase* awaiter)
{
    if(AwaiterBase* t = m_awaiter.load(); !m_awaiter.compare_exchange_strong(t, awaiter)) {
        return false;
    }
    return true;
}

inline void Coroutine<void>::AppendExitCallback(const std::function<void()>& callback)
{
    m_exit_cbs->emplace_front(callback);
}

inline AwaiterBase* Coroutine<void>::GetAwaiter() const
{
    return m_awaiter.load();
}

inline void Coroutine<void>::ToExit()
{
    while(!m_exit_cbs->empty()) {
        m_exit_cbs->front()();
        m_exit_cbs->pop_front();
    }
}



template<typename T, typename CoRtn>
inline AsyncResult<T, CoRtn>::AsyncResult(details::WaitAction* action, void* ctx) 
    : details::Awaiter<T>(action, ctx), m_handle(nullptr) 
{
}

template<typename T, typename CoRtn>
inline AsyncResult<T, CoRtn>::AsyncResult(T result) 
    : details::Awaiter<T>(result), m_handle(nullptr)
{
}

template<typename T, typename CoRtn>
inline bool AsyncResult<T, CoRtn>::await_ready() const noexcept 
{
    return this->m_action == nullptr || !this->m_action->HasEventToDo();
}

template<typename T, typename CoRtn>
inline bool AsyncResult<T, CoRtn>::await_suspend(std::coroutine_handle<typename Coroutine<CoRtn>::promise_type> handle) noexcept 
{
    this->m_handle = handle;
    typename Coroutine<CoRtn>::wptr co = handle.promise().GetCoroutine();
    if(co.expired()) {
        LogError("[Coroutine expired]");
        return false;
    }
    while(!co.lock()->SetAwaiter(this)){
        LogError("[Set awaiter failed]");
    }
    return this->m_action->DoAction(std::static_pointer_cast<CoroutineBase>(co.lock()), this->m_ctx);
}

template<typename T, typename CoRtn>
inline T AsyncResult<T, CoRtn>::await_resume() const noexcept 
{
    if( this->m_handle ) {
        while(!this->m_handle.promise().GetCoroutine().lock()->SetAwaiter(nullptr)){
            LogError("[Set awaiter failed]");
        }
    }
    return this->m_result;
}



template<typename T, typename CoRtn>
inline Coroutine<CoRtn>::wptr AsyncResult<T, CoRtn>::GetCoroutine() const 
{
    if(m_handle) return m_handle.promise().GetCoroutine();
    return {};
}

template<typename CoRtn>
AsyncResult<void, CoRtn>::AsyncResult(details::WaitAction* action, void* ctx) 
    : m_ctx(ctx), m_action(action), m_handle(nullptr) 
{

}

template<typename CoRtn>
AsyncResult<void, CoRtn>::AsyncResult() 
    : m_ctx(nullptr), m_action(nullptr), m_handle(nullptr)
{

}

template<typename CoRtn>
bool AsyncResult<void, CoRtn>::await_ready() const noexcept {
    return m_action == nullptr || !m_action->HasEventToDo();
}

//true will suspend, false will not
template<typename CoRtn>
bool AsyncResult<void, CoRtn>::await_suspend(std::coroutine_handle<typename Coroutine<CoRtn>::promise_type> handle) noexcept {
    m_handle = handle;
    typename Coroutine<CoRtn>::wptr co = handle.promise().GetCoroutine();
    if(co.expired()) {
        LogError("[Coroutine expired]");
        return false;
    }
    while(!co.lock()->SetAwaiter(this)){
        LogError("[Set awaiter failed]");
    }
    return m_action->DoAction(co, m_ctx);
}

template<typename CoRtn>
void AsyncResult<void, CoRtn>::await_resume() const noexcept {
    if( m_handle ) {
        while(!m_handle.promise().GetCoroutine().lock()->SetAwaiter(nullptr)){
            LogError("[Set awaiter failed]");
        }
    }
}

template<typename CoRtn>
typename Coroutine<CoRtn>::wptr AsyncResult<void, CoRtn>::GetCoroutine() const {
    if( m_handle ) {
        return m_handle.promise().GetCoroutine();
    }
    return {};
}

}


#endif