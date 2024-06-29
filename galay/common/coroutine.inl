
#ifndef GALAY_COROUTINE_INL
#define GALAY_COROUTINE_INL
template <typename RESULT>
int 
galay::common::GY_TcpPromise<RESULT>::get_return_object_on_alloaction_failure() noexcept
{
    return -1;
}

template <typename RESULT>
std::coroutine_handle<galay::common::GY_TcpPromise<RESULT>> 
galay::common::GY_TcpPromise<RESULT>::get_return_object() 
{ 
    return std::coroutine_handle<GY_TcpPromise>::from_promise(*this); 
}

template <typename RESULT>
std::suspend_never
galay::common::GY_TcpPromise<RESULT>::initial_suspend() noexcept
{
    return {};
}

template <typename RESULT>
std::suspend_always
galay::common::GY_TcpPromise<RESULT>::yield_value(const RESULT &value)
{
    SetResult(value);
    SetStatus(CoroutineStatus::kCoroutineSuspend);
    return {};
}

template <typename RESULT>
std::suspend_always
galay::common::GY_TcpPromise<RESULT>::yield_value(RESULT value)
{
    SetResult(value);
    SetStatus(CoroutineStatus::kCoroutineSuspend);
    return {};
}

template <typename RESULT>
std::suspend_always
galay::common::GY_TcpPromise<RESULT>::final_suspend() noexcept
{
    SetStatus(CoroutineStatus::kCoroutineFinished);
    return {};
}

template <typename RESULT>
void galay::common::GY_TcpPromise<RESULT>::unhandled_exception() noexcept
{
    m_exception = std::current_exception();
}

template <typename RESULT>
void galay::common::GY_TcpPromise<RESULT>::return_value(RESULT val) noexcept
{
    SetStatus(CoroutineStatus::kCoroutineFinished);
    SetResult(val);
}

template <typename RESULT>
RESULT
galay::common::GY_TcpPromise<RESULT>::GetResult()
{
    rethrow_if_exception();
    return this->m_result;
}

template <typename RESULT>
void galay::common::GY_TcpPromise<RESULT>::SetResult(RESULT result)
{
    this->m_result = result;
}


template <typename RESULT>
galay::common::CoroutineStatus 
galay::common::GY_TcpPromise<RESULT>::GetStatus()
{
    return this->m_status;
}


template <typename RESULT>
void 
galay::common::GY_TcpPromise<RESULT>::SetStatus(CoroutineStatus status)
{
    this->m_status = status;
}

template <typename RESULT>
void galay::common::GY_TcpPromise<RESULT>::rethrow_if_exception()
{
    if (m_exception)
    {
        std::rethrow_exception(m_exception);
    }
}

int 
galay::common::GY_TcpPromise<void>::get_return_object_on_alloaction_failure()
{
    return -1;
}

std::coroutine_handle<galay::common::GY_TcpPromise<void>>
galay::common::GY_TcpPromise<void>::get_return_object()
{
    return std::coroutine_handle<GY_TcpPromise>::from_promise(*this);
}

std::suspend_never
galay::common::GY_TcpPromise<void>::initial_suspend() noexcept
{
    return {};
}

template <typename T>
std::suspend_always 
galay::common::GY_TcpPromise<void>::yield_value(const T &value)
{
    return {};
}

std::suspend_always
galay::common::GY_TcpPromise<void>::final_suspend() noexcept
{
    return {};
}

void 
galay::common::GY_TcpPromise<void>::unhandled_exception() noexcept
{
    m_exception = std::current_exception();
}

galay::common::CoroutineStatus 
galay::common::GY_TcpPromise<void>::GetStatus()
{
    return this->m_status;
}

void 
galay::common::GY_TcpPromise<void>::SetStatus(CoroutineStatus status)
{
    this->m_status = status;
}

void galay::common::GY_TcpPromise<void>::rethrow_if_exception()
{
    if (m_exception)
    {
        std::rethrow_exception(m_exception);
    }
}


//suspend
template <typename RESULT>
int 
galay::common::GY_TcpPromiseSuspend<RESULT>::get_return_object_on_alloaction_failure() noexcept
{
    return -1;
}

template <typename RESULT>
std::coroutine_handle<galay::common::GY_TcpPromiseSuspend<RESULT>> 
galay::common::GY_TcpPromiseSuspend<RESULT>::get_return_object() 
{ 
    return std::coroutine_handle<GY_TcpPromiseSuspend>::from_promise(*this); 
}

template <typename RESULT>
std::suspend_always
galay::common::GY_TcpPromiseSuspend<RESULT>::initial_suspend() noexcept
{
    return {};
}

template <typename RESULT>
std::suspend_always
galay::common::GY_TcpPromiseSuspend<RESULT>::yield_value(const RESULT &value)
{
    SetResult(value);
    SetStatus(CoroutineStatus::kCoroutineSuspend);
    return {};
}

template <typename RESULT>
std::suspend_always
galay::common::GY_TcpPromiseSuspend<RESULT>::yield_value(RESULT value)
{
    SetResult(value);
    SetStatus(CoroutineStatus::kCoroutineSuspend);
    return {};
}

template <typename RESULT>
std::suspend_always
galay::common::GY_TcpPromiseSuspend<RESULT>::final_suspend() noexcept
{
    SetStatus(CoroutineStatus::kCoroutineFinished);
    return {};
}

template <typename RESULT>
void galay::common::GY_TcpPromiseSuspend<RESULT>::unhandled_exception() noexcept
{
    m_exception = std::current_exception();
}

template <typename RESULT>
void galay::common::GY_TcpPromiseSuspend<RESULT>::return_value(RESULT val) noexcept
{
    SetStatus(CoroutineStatus::kCoroutineFinished);
    SetResult(val);
}

template <typename RESULT>
RESULT
galay::common::GY_TcpPromiseSuspend<RESULT>::GetResult()
{
    rethrow_if_exception();
    return this->m_result;
}

template <typename RESULT>
void galay::common::GY_TcpPromiseSuspend<RESULT>::SetResult(RESULT result)
{
    this->m_result = result;
}

template <typename RESULT>
galay::common::CoroutineStatus 
galay::common::GY_TcpPromiseSuspend<RESULT>::GetStatus()
{
    return this->m_status;
}


template <typename RESULT>
void 
galay::common::GY_TcpPromiseSuspend<RESULT>::SetStatus(CoroutineStatus status)
{
    this->m_status = status;
}

template <typename RESULT>
void galay::common::GY_TcpPromiseSuspend<RESULT>::rethrow_if_exception()
{
    if (m_exception)
    {
        std::rethrow_exception(m_exception);
    }
}

int 
galay::common::GY_TcpPromiseSuspend<void>::get_return_object_on_alloaction_failure()
{
    return -1;
}

std::coroutine_handle<galay::common::GY_TcpPromiseSuspend<void>>
galay::common::GY_TcpPromiseSuspend<void>::get_return_object()
{
    return std::coroutine_handle<GY_TcpPromiseSuspend>::from_promise(*this);
}

std::suspend_always
galay::common::GY_TcpPromiseSuspend<void>::initial_suspend() noexcept
{
    return {};
}

template <typename T>
std::suspend_always 
galay::common::GY_TcpPromiseSuspend<void>::yield_value(const T &value)
{
    return {};
}

std::suspend_always
galay::common::GY_TcpPromiseSuspend<void>::final_suspend() noexcept
{
    return {};
}

void 
galay::common::GY_TcpPromiseSuspend<void>::unhandled_exception() noexcept
{
    m_exception = std::current_exception();
}

galay::common::CoroutineStatus 
galay::common::GY_TcpPromiseSuspend<void>::GetStatus()
{
    return this->m_status;
}

void 
galay::common::GY_TcpPromiseSuspend<void>::SetStatus(CoroutineStatus status)
{
    this->m_status = status;
}

void 
galay::common::GY_TcpPromiseSuspend<void>::rethrow_if_exception()
{
    if (m_exception)
    {
        std::rethrow_exception(m_exception);
    }
}

//end

//不能使用std::move 本质上m_handle是协程的引用，无法移交所有权，需要手动移动
template <typename RESULT>
galay::common::GY_Coroutine<RESULT>& 
galay::common::GY_Coroutine<RESULT>::operator=(GY_Coroutine<RESULT> &&other)
{
    if(this != &other){
        if(this->m_handle){
            this->m_handle.destroy();
        }
        this->m_handle = other.m_handle;
        other.m_handle = {};
    }
    return *this;
}

template <typename RESULT>
galay::common::GY_Coroutine<RESULT>::GY_Coroutine(std::coroutine_handle<promise_type> co_handle) noexcept
{
    this->m_handle = co_handle;
}

template <typename RESULT>
galay::common::GY_Coroutine<RESULT>::GY_Coroutine(GY_Coroutine<RESULT> &&other) noexcept
{
    if(this != &other) {
        if(this->m_handle){
            this->m_handle.destroy();
        }
        this->m_handle = other.m_handle;
        other.m_handle = {};
    }
}

template <typename RESULT>
void 
galay::common::GY_Coroutine<RESULT>::Resume() noexcept
{
    m_handle.resume();
}

template <typename RESULT>
bool 
galay::common::GY_Coroutine<RESULT>::Done() noexcept
{
    return m_handle.done();
}

template <typename RESULT>
galay::common::GY_Coroutine<RESULT>::~GY_Coroutine()
{
    if (m_handle)
    {
        m_handle.destroy();
        m_handle = nullptr;
    }
}


template <typename RESULT>
galay::common::GY_TcpCoroutine<RESULT>::GY_TcpCoroutine(std::coroutine_handle<promise_type> co_handle) noexcept
    : GY_Coroutine<RESULT>(co_handle)
{
}

template <typename RESULT>
galay::common::GY_TcpCoroutine<RESULT>::GY_TcpCoroutine(GY_TcpCoroutine<RESULT> &&other) noexcept
    : GY_Coroutine<RESULT>(std::forward<GY_TcpCoroutine<RESULT> &&>(other))
{
}

template <typename RESULT>
galay::common::GY_TcpCoroutine<RESULT> &galay::common::GY_TcpCoroutine<RESULT>::operator=(GY_TcpCoroutine<RESULT> &&other)
{
    GY_Coroutine<RESULT>::operator=(std::forward<GY_TcpCoroutine<RESULT> &&>(other));
    return *this;
}

template <typename RESULT>
bool 
galay::common::GY_TcpCoroutine<RESULT>::IsCoroutine()
{
    return this->m_handle != nullptr;
}

template <typename RESULT>
std::coroutine_handle<typename galay::common::GY_TcpCoroutine<RESULT>::promise_type> 
galay::common::GY_TcpCoroutine<RESULT>::GetCoroutine() const
{
    return this->m_handle;
}

template <typename RESULT>
RESULT
galay::common::GY_TcpCoroutine<RESULT>::GetResult()
{
    if (!this->m_handle)
        return {};
    return this->m_handle.promise().GetResult();
}

template <typename RESULT>
void galay::common::GY_TcpCoroutine<RESULT>::SetResult(RESULT result)
{
    if (this->m_handle)
        this->m_handle.promise().SetResult(result);
}

template <typename RESULT>
galay::common::CoroutineStatus 
galay::common::GY_TcpCoroutine<RESULT>::GetStatus()
{
    if (!this->m_handle) return CoroutineStatus::kCoroutineNotExist;
    return this->m_handle.promise().GetStatus();
}

template <typename RESULT>
void 
galay::common::GY_TcpCoroutine<RESULT>::SetStatus(CoroutineStatus status)
{
    if (this->m_handle)
        this->m_handle.promise().SetStatus(status);
}


template <typename RESULT>
galay::common::GY_TcpCoroutine<RESULT>::~GY_TcpCoroutine()
{
    
}

//suspend

template <typename RESULT>
galay::common::GY_CoroutineSuspend<RESULT>& 
galay::common::GY_CoroutineSuspend<RESULT>::operator=(GY_CoroutineSuspend<RESULT> &&other)
{
    if(this != &other){
        if(this->m_handle){
            this->m_handle.destroy();
        }
        this->m_handle = other.m_handle;
        other.m_handle = {};
    }
    return *this;
}

template <typename RESULT>
galay::common::GY_CoroutineSuspend<RESULT>::GY_CoroutineSuspend(std::coroutine_handle<promise_type> co_handle) noexcept
{
    this->m_handle = co_handle;
}

template <typename RESULT>
galay::common::GY_CoroutineSuspend<RESULT>::GY_CoroutineSuspend(GY_CoroutineSuspend<RESULT> &&other) noexcept
{
    if(this != &other) {
        if(this->m_handle){
            this->m_handle.destroy();
        }
        this->m_handle = other.m_handle;
        other.m_handle = {};
    }
}

template <typename RESULT>
void 
galay::common::GY_CoroutineSuspend<RESULT>::Resume() noexcept
{
    m_handle.resume();
}

template <typename RESULT>
bool 
galay::common::GY_CoroutineSuspend<RESULT>::Done() noexcept
{
    return m_handle.done();
}

template <typename RESULT>
galay::common::GY_CoroutineSuspend<RESULT>::~GY_CoroutineSuspend()
{
    if (m_handle)
    {
        m_handle.destroy();
        m_handle = nullptr;
    }
}

template <typename RESULT>
galay::common::GY_TcpCoroutineSuspend<RESULT>::GY_TcpCoroutineSuspend(std::coroutine_handle<promise_type> co_handle) noexcept
    : GY_CoroutineSuspend<RESULT>(co_handle)
{
}

template <typename RESULT>
galay::common::GY_TcpCoroutineSuspend<RESULT>::GY_TcpCoroutineSuspend(GY_TcpCoroutineSuspend<RESULT> &&other) noexcept
    : GY_CoroutineSuspend<RESULT>(std::forward<GY_TcpCoroutineSuspend<RESULT> &&>(other))
{
}

template <typename RESULT>
galay::common::GY_TcpCoroutineSuspend<RESULT> &galay::common::GY_TcpCoroutineSuspend<RESULT>::operator=(GY_TcpCoroutineSuspend<RESULT> &&other)
{
    GY_CoroutineSuspend<RESULT>::operator=(std::forward<GY_TcpCoroutineSuspend<RESULT> &&>(other));
    return *this;
}

template <typename RESULT>
bool 
galay::common::GY_TcpCoroutineSuspend<RESULT>::IsCoroutine()
{
    return this->m_handle != nullptr;
}

template <typename RESULT>
std::coroutine_handle<typename galay::common::GY_TcpCoroutineSuspend<RESULT>::promise_type> 
galay::common::GY_TcpCoroutineSuspend<RESULT>::GetCoroutine() const
{
    return this->m_handle;
}

template <typename RESULT>
RESULT
galay::common::GY_TcpCoroutineSuspend<RESULT>::GetResult()
{
    if (!this->m_handle)
        return {};
    return this->m_handle.promise().GetResult();
}

template <typename RESULT>
void galay::common::GY_TcpCoroutineSuspend<RESULT>::SetResult(RESULT result)
{
    if (this->m_handle)
        this->m_handle.promise().SetResult(result);
}

template <typename RESULT>
galay::common::CoroutineStatus 
galay::common::GY_TcpCoroutineSuspend<RESULT>::GetStatus()
{
    if (!this->m_handle) return CoroutineStatus::kCoroutineNotExist;
    return this->m_handle.promise().GetStatus();
}

template <typename RESULT>
void 
galay::common::GY_TcpCoroutineSuspend<RESULT>::SetStatus(CoroutineStatus status)
{
    if (this->m_handle)
        this->m_handle.promise().SetStatus(status);
}


template <typename RESULT>
galay::common::GY_TcpCoroutineSuspend<RESULT>::~GY_TcpCoroutineSuspend()
{
    
}


template class galay::common::GY_TcpCoroutine<galay::common::CoroutineStatus>;
template class galay::common::GY_TcpCoroutineSuspend<galay::common::CoroutineStatus>;

#endif
