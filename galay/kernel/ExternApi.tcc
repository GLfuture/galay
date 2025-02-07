#ifndef GALAY_EXTERNAPI_TCC
#define GALAY_EXTERNAPI_TCC

#include "ExternApi.hpp"

namespace galay 
{

template <VecBase IOVecType>
inline IOVecHolder<IOVecType>::IOVecHolder(size_t size) 
{ 
    VecMalloc(&m_vec, size); 
}


template <VecBase IOVecType>
inline IOVecHolder<IOVecType>::IOVecHolder(std::string&& buffer) 
{
    if(!buffer.empty()) {
        Reset(std::move(buffer));
    }
}

template <VecBase IOVecType>
inline bool IOVecHolder<IOVecType>::Realloc(size_t size)
{
    return VecRealloc(&m_vec, size);
}

template <VecBase IOVecType>
inline void IOVecHolder<IOVecType>::ClearBuffer()
{
    if(m_vec.m_buffer == nullptr) return;
    memset(m_vec.m_buffer, 0, m_vec.m_size);
    m_vec.m_offset = 0;
    m_vec.m_length = 0;
}

template <VecBase IOVecType>
inline bool IOVecHolder<IOVecType>::Reset(const IOVecType& iov)
{
    if(iov.m_buffer == nullptr) return false;
    FreeMemory();
    m_vec.m_buffer = iov.m_buffer;
    m_vec.m_size = iov.m_size;
    m_vec.m_offset = iov.m_offset;
    m_vec.m_length = iov.m_length;
    return true;
}

template <VecBase IOVecType>
inline bool IOVecHolder<IOVecType>::Reset()
{
    return FreeMemory();
}

template <VecBase IOVecType>
inline bool IOVecHolder<IOVecType>::Reset(std::string&& str)
{
    FreeMemory();
    m_temp = std::forward<std::string>(str);
    m_vec.m_buffer = m_temp.data();
    m_vec.m_size = m_temp.length();
    return true;
}

template <VecBase IOVecType>
inline IOVecType* IOVecHolder<IOVecType>::operator->()
{
    return &m_vec;
}

template <VecBase IOVecType>
inline IOVecType* IOVecHolder<IOVecType>::operator &()
{
    return &m_vec;
}

template <VecBase IOVecType>
inline IOVecHolder<IOVecType>::~IOVecHolder()
{
    FreeMemory();    
}

template <VecBase IOVecType>
inline bool IOVecHolder<IOVecType>::VecMalloc(IOVecType* src, size_t size)
{
    if (src == nullptr || src->m_size != 0 || src->m_buffer != nullptr) {
        return false;
    }
    src->m_buffer = static_cast<char*>(calloc(size, sizeof(char)));
    if (src->m_buffer == nullptr) {
        return false;
    }
    src->m_size = size;
    return true;
}

template <VecBase IOVecType>
inline bool IOVecHolder<IOVecType>::VecRealloc(IOVecType* src, size_t size)
{
    if (src == nullptr || src->m_buffer == nullptr) {
        return false;
    }
    src->m_buffer = static_cast<char*>(realloc(src->m_buffer, size));
    if (src->m_buffer == nullptr) {
        return false;
    }
    src->m_size = size;
    return true;
}

template <VecBase IOVecType>
inline bool IOVecHolder<IOVecType>::VecFree(IOVecType* src)
{
    if(src == nullptr || src->m_size == 0 || src->m_buffer == nullptr) {
        return false;
    }
    free(src->m_buffer);
    src->m_size = 0;
    src->m_length = 0;
    src->m_buffer = nullptr;
    src->m_offset = 0;
    return true;
}


template <VecBase IOVecType>
inline bool IOVecHolder<IOVecType>::FreeMemory() 
{
    bool res = false;
    if(m_temp.empty()) {
        return VecFree(&m_vec);
    } else {
        if(m_temp.data() != m_vec.m_buffer) {
            res = VecFree(&m_vec);
        }
    }
    m_temp.clear();
    return true;
}

template<>
inline bool IOVecHolder<UdpIOVec>::Reset(const UdpIOVec& iov)
{
    if(iov.m_buffer == nullptr) return false;
    FreeMemory();
    m_vec.m_buffer = iov.m_buffer;
    m_vec.m_size = iov.m_size;
    m_vec.m_offset = iov.m_offset;
    m_vec.m_length = iov.m_length;
    m_vec.m_addr = iov.m_addr;
    return true;
}


template<typename CoRtn>
inline WaitGroup<CoRtn>::WaitGroup(uint32_t count)
    :m_count(count)
{
}

template<typename CoRtn>
inline void WaitGroup<CoRtn>::Done()
{
    std::shared_lock lk(m_mutex);
    if(m_count == 0) return;
    --m_count;
    if(m_count == 0) {
        if(!m_coroutine.expired()) {
            static_cast<AsyncResult<bool, CoRtn>*>(m_coroutine.lock()->GetAwaiter())->SetResult(true);
            m_coroutine.lock()->GetCoScheduler()->ToResumeCoroutine(m_coroutine);
        }
    }
}

template<typename CoRtn>
inline void WaitGroup<CoRtn>::Reset(uint32_t count)
{
    std::unique_lock lk(m_mutex);
    m_count = count;
    m_coroutine = {};
}

template<typename CoRtn>
inline AsyncResult<bool, CoRtn> WaitGroup<CoRtn>::Wait()
{
    auto* action = new details::CoroutineHandleAction([this](CoroutineBase::wptr co, void* ctx)->bool{
        std::unique_lock lk(m_mutex);
        if(m_count == 0) {
            if(!co.expired()) static_cast<AsyncResult<bool, CoRtn>*>(co.lock()->GetAwaiter())->SetResult(false);
            return false;
        }
        m_coroutine = co;
        return true;
    });
    return {action, nullptr};
}


}

namespace galay::this_coroutine 
{

template<typename CoRtn>
AsyncResult<CoroutineBase::wptr, CoRtn> GetThisCoroutine()
{
    auto action = new details::CoroutineHandleAction([](CoroutineBase::wptr co, void* ctx)->bool{
        static_cast<AsyncResult<CoroutineBase::wptr, CoRtn>*>(co.lock()->GetAwaiter())->SetResult(co);
        return false;
    });
    return {action, nullptr};
}


template<typename CoRtn>
AsyncResult<void, CoRtn> Sleepfor(int64_t ms)
{
    auto func = [ms](CoroutineBase::wptr co, void* ctx)->bool{
        if(ms <= 0) return false;
        auto timecb = [co](details::AbstractTimeEvent::wptr event, Timer::ptr timer){
            if(!co.expired()) co.lock()->GetCoScheduler()->ToResumeCoroutine(co);
        };
        auto timer = std::make_shared<Timer>(std::move(timecb));
        co.lock()->GetEventScheduler()->AddTimer(timer, ms);
        return true;
    };
    auto* action = new details::CoroutineHandleAction(std::move(func));
    return {action , nullptr};
}

template <typename CoRtn>
AsyncResult<void, CoRtn> Sleepfor(details::EventScheduler *scheduler, int64_t ms)
{
    auto func = [scheduler, ms](CoroutineBase::wptr co, void* ctx)->bool{
        if(ms <= 0) return false;
        auto timecb = [co](details::AbstractTimeEvent::wptr event, Timer::ptr timer){
            if(!co.expired()) co.lock()->GetCoScheduler()->ToResumeCoroutine(co);
        };
        auto timer = std::make_shared<Timer>(ms, std::move(timecb));
        scheduler->AddTimer(ms,  timer);
        return true;
    };
    auto* action = new details::CoroutineHandleAction(std::move(func));
    return {action , nullptr};
}

template<typename CoRtn>
AsyncResult<void, CoRtn> DeferExit(const std::function<void(void)>& callback)
{
    auto* action = new details::CoroutineHandleAction([callback](CoroutineBase::wptr co, void* ctx)->bool{
        co.lock()->AppendExitCallback(std::move(callback));
        return false;
    });
    return {action, nullptr};
}

template<typename CoRtn, typename FCoRtn>
AsyncResult<typename Coroutine<CoRtn>::ptr, FCoRtn> WaitAsyncExecute(Coroutine<CoRtn>&& co)
{
    auto action = new details::CoroutineHandleAction([co](CoroutineBase::wptr fco, void* ctx)->bool {
        auto child = co;
        auto awaiter = static_cast<typename details::Awaiter<typename Coroutine<CoRtn>::ptr>*>(std::dynamic_pointer_cast<Coroutine<FCoRtn>>(fco.lock())->GetAwaiter());
        awaiter->SetResult(std::make_shared<Coroutine<CoRtn>>(child));
        if(child.IsDone()) return false;
        child.AppendExitCallback([fco](){
            fco.lock()->GetCoScheduler()->ToResumeCoroutine(fco);
        });
        return true;
    });
    return {action, nullptr};
}

template<typename CoRtn, typename FCoRtn, AsyncFuncType<CoRtn> Func, RoutineCtxType FirstArg, typename ...Args>
galay::AsyncResult<typename Coroutine<CoRtn>::ptr, FCoRtn> WaitAsyncExecute(Func func, FirstArg first, Args&&... args)
{
    auto async_func = std::bind(func, std::forward<FirstArg>(first), std::forward<Args>(args)...);
    auto action = new galay::details::CoroutineHandleAction([async_func](galay::CoroutineBase::wptr co, void *ctx){
        auto coro = async_func();
        auto awaiter = static_cast<typename details::Awaiter<typename Coroutine<CoRtn>::ptr>*>(std::dynamic_pointer_cast<Coroutine<FCoRtn>>(co.lock())->GetAwaiter());
        awaiter->SetResult(std::make_shared<Coroutine<CoRtn>>(coro));
        if(coro.IsDone()) return false;
        coro.AppendExitCallback([co](){
            co.lock()->GetCoScheduler()->ToResumeCoroutine(co);
        });
        return true;
    });
    return {action, nullptr};
}


template<typename CoRtn,  typename FCoRtn, typename Class, CoroutineType<CoRtn> Ret, RoutineCtxType FirstArg, typename ...FuncArgs, typename ...Args>
galay::AsyncResult<typename Coroutine<CoRtn>::ptr, FCoRtn> WaitAsyncExecute(Ret(Class::*func)(FirstArg, FuncArgs...), Class* obj, FirstArg first, Args&&... args)
{
    auto async_func = std::bind(func, obj, std::forward<FirstArg>(first), std::forward<Args>(args)...);
    auto action = new galay::details::CoroutineHandleAction([async_func](galay::CoroutineBase::wptr co, void *ctx){
        auto coro = async_func();
        auto awaiter = static_cast<typename details::Awaiter<typename Coroutine<CoRtn>::ptr>*>(std::dynamic_pointer_cast<Coroutine<FCoRtn>>(co.lock())->GetAwaiter());
        awaiter->SetResult(std::make_shared<Coroutine<CoRtn>>(coro));
        if(coro.IsDone()) return false;
        coro.AppendExitCallback([co](){
            co.lock()->GetCoScheduler()->ToResumeCoroutine(co);
        });
        return true;
    });
    return {action, nullptr};
}


}


#endif