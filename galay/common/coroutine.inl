
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
    SetStatus(CoroutineStatus::kCoroutineRunning);
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
    if(m_finishFunc != nullptr) m_finishFunc();
}

template <typename RESULT>
RESULT
galay::common::GY_TcpPromise<RESULT>::GetResult()
{
    rethrow_if_exception();
    return this->m_result;
}

template <typename RESULT>
void 
galay::common::GY_TcpPromise<RESULT>::SetResult(RESULT result)
{
    this->m_result = result;
}

template <typename RESULT>
void 
galay::common::GY_TcpPromise<RESULT>::SetFinishFunc(std::function<void()> finshfunc)
{
    this->m_finishFunc = finshfunc;
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
    SetStatus(CoroutineStatus::kCoroutineRunning);
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
    SetStatus(CoroutineStatus::kCoroutineSuspend);
    return {};
}

std::suspend_always
galay::common::GY_TcpPromise<void>::final_suspend() noexcept
{
    SetStatus(CoroutineStatus::kCoroutineFinished);
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

void 
galay::common::GY_TcpPromise<void>::SetFinishFunc(std::function<void()> finshfunc)
{
    this->m_finishFunc = finshfunc;
}

void galay::common::GY_TcpPromise<void>::rethrow_if_exception()
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
uint64_t 
galay::common::GY_Coroutine<RESULT>::GetCoId() const noexcept
{
    if(m_handle == nullptr) return 0;
    return reinterpret_cast<uint64_t>(m_handle.address());
}

template <typename RESULT>
galay::common::GY_Coroutine<RESULT>::~GY_Coroutine()
{
    if (m_handle)
    {
        spdlog::info("[{}:{}] [~GY_Coroutine() CoId = {}]", __FILE__, __LINE__, GetCoId());
        m_handle.destroy();
        m_handle = nullptr;
    }
}


template <typename RESULT>
galay::common::GY_NetCoroutine<RESULT>::GY_NetCoroutine(std::coroutine_handle<promise_type> co_handle) noexcept
    : GY_Coroutine<RESULT>(co_handle)
{
}

template <typename RESULT>
galay::common::GY_NetCoroutine<RESULT>::GY_NetCoroutine(GY_NetCoroutine<RESULT> &&other) noexcept
    : GY_Coroutine<RESULT>(std::forward<GY_NetCoroutine<RESULT> &&>(other))
{
}

template <typename RESULT>
galay::common::GY_NetCoroutine<RESULT> &galay::common::GY_NetCoroutine<RESULT>::operator=(GY_NetCoroutine<RESULT> &&other)
{
    GY_Coroutine<RESULT>::operator=(std::forward<GY_NetCoroutine<RESULT> &&>(other));
    return *this;
}

template <typename RESULT>
bool 
galay::common::GY_NetCoroutine<RESULT>::IsCoroutine()
{
    return this->m_handle != nullptr;
}


template <typename RESULT>
RESULT
galay::common::GY_NetCoroutine<RESULT>::GetResult()
{
    if (!this->m_handle)
        return {};
    return this->m_handle.promise().GetResult();
}


template <typename RESULT>
galay::common::CoroutineStatus 
galay::common::GY_NetCoroutine<RESULT>::GetStatus()
{
    if (!this->m_handle) return CoroutineStatus::kCoroutineNotExist;
    return this->m_handle.promise().GetStatus();
}


template <typename RESULT>
galay::common::GY_NetCoroutine<RESULT>::~GY_NetCoroutine()
{
    
}

template class galay::common::GY_NetCoroutine<galay::common::CoroutineStatus>;

#endif
