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
inline Awaiter<T>::Awaiter(T&& result)
    : m_result(std::move(result)), m_action(nullptr), m_ctx(nullptr)
{
}

template <typename T>
inline void Awaiter<T>::SetResult(T &&result)
{
    m_result = std::move(result);
}

}

namespace galay
{

template <typename T>
inline T *CoroutineBase::ConvertTo()
{
    return static_cast<T*>(this);
}
    

template <typename T>
template <typename...Args>
inline PromiseType<T>::PromiseType(RoutineCtx ctx, Args&&...args)
    : m_ctx(std::move(ctx))
{
}

template <typename T>
template <typename... Args>
inline PromiseType<T>::PromiseType(void *ptr, RoutineCtx ctx, Args &&...agrs)
    : m_ctx(std::move(ctx))
{
}

template <typename T>
inline Coroutine<T> PromiseType<T>::get_return_object() noexcept
{
    m_coroutine = std::make_shared<Coroutine<T>>(std::coroutine_handle<PromiseType>::from_promise(*this));
    return *m_coroutine;
}

template<typename T>
inline std::suspend_always PromiseType<T>::yield_value(T&& value) noexcept 
{ 
    *(m_coroutine->m_result) = std::move(value); 
    m_coroutine->Become(CoroutineStatus::Suspended);
    return {}; 
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
    CoroutineStatus origin = m_coroutine->m_status->load();
    if(!m_coroutine->m_status->compare_exchange_strong(origin, CoroutineStatus::Finished)) {
        return;
    }
    if(m_coroutine) {
        m_coroutine->ToExit();
    }
}

template<typename ...Args>
inline PromiseType<void>::PromiseType(RoutineCtx ctx, Args&&... agrs)
    : m_ctx(std::move(ctx))
{
}

template<typename ...Args>
inline PromiseType<void>::PromiseType(void* ptr, RoutineCtx ctx, Args&&... agrs)
    : m_ctx(std::move(ctx))
{
}

inline Coroutine<void> PromiseType<void>::get_return_object() noexcept
{
    m_coroutine = std::make_shared<Coroutine<void>>(std::coroutine_handle<PromiseType<void>>::from_promise(*this));
    return *m_coroutine;
}

inline std::suspend_always PromiseType<void>::yield_value(std::monostate) noexcept
{
    m_coroutine->Become(CoroutineStatus::Suspended);
    return std::suspend_always();
}

inline PromiseType<void>::~PromiseType()
{
    CoroutineStatus origin = m_coroutine->m_status->load();
    if(!m_coroutine->m_status->compare_exchange_strong(origin, CoroutineStatus::Finished)) {
        return;
    }
    if(m_coroutine) {
        m_coroutine->ToExit();
    }
}


template<typename T>
inline Coroutine<T>::Coroutine(const std::coroutine_handle<promise_type> handle) noexcept
{
    m_status = std::make_shared<std::atomic<CoroutineStatus>>(CoroutineStatus::Running);
    m_result = std::make_shared<std::optional<T>>();
    m_handle = handle;
    m_awaiter = nullptr;
    m_exit_cbs = std::make_shared<std::list<ExitHandle>>();
}

template<typename T>
inline Coroutine<T>::Coroutine(Coroutine&& other) noexcept
{
    m_handle = other.m_handle;
    other.m_handle = nullptr;
    m_awaiter.store(other.m_awaiter);
    other.m_awaiter = nullptr;
    m_status = other.m_status;
    other.m_status.reset();
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
    m_status = other.m_status;
    m_result = other.m_result;
}

template<typename T>
inline Coroutine<T>& Coroutine<T>::operator=(Coroutine&& other) noexcept
{
    m_handle = other.m_handle;
    other.m_handle = nullptr;
    m_awaiter.store(other.m_awaiter);
    other.m_awaiter = nullptr;
    m_status = other.m_status;
    other.m_status.reset();
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
    m_status = other.m_status;
    m_handle = other.m_handle;
    m_exit_cbs = other.m_exit_cbs;
    m_result = other.m_result;
    return *this;
}

template<typename T>
inline details::CoroutineScheduler* Coroutine<T>::GetCoScheduler() const
{
    if(m_status->load() == CoroutineStatus::Finished) {
        return nullptr;
    }
    return m_handle.promise().m_ctx.GetSharedCtx().lock()->GetCoScheduler();
}

template <typename T>
inline details::EventScheduler *Coroutine<T>::GetEventScheduler() const
{
    if(m_status->load() == CoroutineStatus::Finished) {
        return nullptr;
    }
    return m_handle.promise().m_ctx.GetSharedCtx().lock()->GetEventScheduler();
}

template<typename T>
inline bool Coroutine<T>::IsRunning() const
{
    return m_status->load() == CoroutineStatus::Running;
}

template<typename T>
inline bool Coroutine<T>::IsSuspend() const 
{ 
    return m_status->load() == CoroutineStatus::Suspended; 
}

template<typename T>
inline bool Coroutine<T>::IsDone() const  
{ 
    return m_status->load() == CoroutineStatus::Finished; 
}

template<typename T>
inline bool Coroutine<T>::SetAwaiter(AwaiterBase* awaiter)
{
    if(AwaiterBase* t = m_awaiter.load(); !m_awaiter.compare_exchange_strong(t, awaiter)) {
        return false;
    }
    return true;
}


template <typename T>
inline void Coroutine<T>::AppendExitCallback(const ExitHandle &callback)
{
    m_exit_cbs->emplace_front(callback);
}

template <typename T>
inline std::optional<T> Coroutine<T>::Result()
{
    return *(m_result);
}

template <typename T>
inline bool Coroutine<T>::Become(CoroutineStatus status)
{
    auto origin = m_status->load();
    if(!m_status->compare_exchange_strong(origin, status)) {
        return false;
    }
    return true;
}

template<typename T>
inline std::optional<T> Coroutine<T>::operator()() 
{ 
    return *m_result; 
}

template<typename T>
inline AwaiterBase* Coroutine<T>::GetAwaiter() const
{
    return m_awaiter.load();
}


template<typename T>
inline void Coroutine<T>::Destroy() 
{ 
    m_handle.destroy(); 
}

template<typename T>
inline void Coroutine<T>::Resume() 
{ 
    return m_handle.resume(); 
}


template<typename T>
inline void Coroutine<T>::ToExit()
{
    while(!m_exit_cbs->empty()) {
        m_exit_cbs->front()(weak_from_this());
        m_exit_cbs->pop_front();
    }
}

inline Coroutine<void>::Coroutine(const std::coroutine_handle<promise_type> handle) noexcept
{
    m_status = std::make_shared<std::atomic<CoroutineStatus>>(CoroutineStatus::Running);
    m_exit_cbs = std::make_shared<std::list<ExitHandle>>();
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
    m_status = other.m_status;
    other.m_status.reset();
}

inline Coroutine<void>::Coroutine(const Coroutine& other) noexcept
{
    m_awaiter.store(other.m_awaiter);
    m_handle = other.m_handle;
    m_exit_cbs = other.m_exit_cbs;
    m_status = other.m_status;
}

inline Coroutine<void>& Coroutine<void>::operator=(Coroutine&& other) noexcept
{
    m_handle = other.m_handle;
    other.m_handle = nullptr;
    m_awaiter.store(other.m_awaiter);
    other.m_awaiter = nullptr;
    m_status = other.m_status;
    other.m_status.reset();
    m_exit_cbs = other.m_exit_cbs;
    other.m_exit_cbs.reset();
    return *this;
}

inline Coroutine<void>& Coroutine<void>::operator=(const Coroutine &other) noexcept
{
    m_awaiter.store(other.m_awaiter);
    m_handle = other.m_handle;
    m_exit_cbs = other.m_exit_cbs;
    m_status = other.m_status;
    return *this;
}

inline details::CoroutineScheduler* Coroutine<void>::GetCoScheduler() const
{
    if(m_status->load() == CoroutineStatus::Finished) {
        return nullptr;
    }
    return m_handle.promise().m_ctx.GetSharedCtx().lock()->GetCoScheduler();
}

inline details::EventScheduler* Coroutine<void>::GetEventScheduler() const
{
    if(m_status->load() == CoroutineStatus::Finished) {
        return nullptr;
    }
    return m_handle.promise().m_ctx.GetSharedCtx().lock()->GetEventScheduler();
}

inline bool Coroutine<void>::IsRunning() const
{
    return m_status->load() == CoroutineStatus::Running;
}

inline bool Coroutine<void>::IsSuspend() const 
{ 
    return m_status->load() == CoroutineStatus::Suspended; 
}

inline bool Coroutine<void>::IsDone() const 
{ 
    return m_status->load() == CoroutineStatus::Finished; 
}

inline bool Coroutine<void>::SetAwaiter(AwaiterBase* awaiter)
{
    if(AwaiterBase* t = m_awaiter.load(); !m_awaiter.compare_exchange_strong(t, awaiter)) {
        return false;
    }
    return true;
}

inline std::optional<std::monostate> Coroutine<void>::Result()
{
    return std::nullopt;
}

inline void Coroutine<void>::AppendExitCallback(const ExitHandle& callback)
{
    m_exit_cbs->emplace_front(callback);
}

inline bool Coroutine<void>::Become(CoroutineStatus status) 
{
    auto origin = m_status->load();
    if(!m_status->compare_exchange_strong(origin, status)) {
        return false;
    }
    return true;
}

inline void Coroutine<void>::Destroy() 
{ 
    m_handle.destroy(); 
}

inline void Coroutine<void>::Resume() 
{ 
    return m_handle.resume(); 
}

inline AwaiterBase* Coroutine<void>::GetAwaiter() const
{
    return m_awaiter.load();
}


inline void Coroutine<void>::ToExit()
{
    while(!m_exit_cbs->empty()) {
        m_exit_cbs->front()(weak_from_this());
        m_exit_cbs->pop_front();
    }
}



template<typename T, typename CoRtn>
inline AsyncResult<T, CoRtn>::AsyncResult(details::WaitAction* action, void* ctx) 
    : details::Awaiter<T>(action, ctx), m_handle(nullptr) 
{
}

template<typename T, typename CoRtn>
inline AsyncResult<T, CoRtn>::AsyncResult(T&& result) 
    : details::Awaiter<T>(std::forward<T>(result)), m_handle(nullptr)
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
    auto isSuspend = this->m_action->DoAction(std::static_pointer_cast<CoroutineBase>(co.lock()), this->m_ctx);
    if(isSuspend) {
        while(!co.lock()->Become(CoroutineStatus::Suspended)) {
            LogError("[Become failed]");
        } 
    }
    return isSuspend;
}

template<typename T, typename CoRtn>
inline T&& AsyncResult<T, CoRtn>::await_resume() const noexcept 
{
    if( this->m_handle ) {
        while(!this->m_handle.promise().GetCoroutine().lock()->SetAwaiter(nullptr)){
            LogError("[Set awaiter failed]");
        }
        while(!this->m_handle.promise().GetCoroutine().lock()->Become(CoroutineStatus::Running)) {
            LogError("[Become failed]");
        } 
    }
    return const_cast<T&&>(std::move(this->m_result));
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
    bool isSuspend = m_action->DoAction(co, m_ctx);
    if(isSuspend) {
        while(!co.lock()->Become(CoroutineStatus::Suspended)) {
            LogError("[Become failed]");
        } 
    }
    return isSuspend;
}

template<typename CoRtn>
void AsyncResult<void, CoRtn>::await_resume() const noexcept {
    if( m_handle ) {
        while(!m_handle.promise().GetCoroutine().lock()->SetAwaiter(nullptr)){
            LogError("[Set awaiter failed]");
        }
        while(!this->m_handle.promise().GetCoroutine().lock()->Become(CoroutineStatus::Running)) {
            LogError("[Become failed]");
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