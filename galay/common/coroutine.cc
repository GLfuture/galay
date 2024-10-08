
#include "coroutine.h"
#include <spdlog/spdlog.h>

std::atomic_char16_t galay::coroutine::NetCoroutinePool::m_threadNum = DEFAULT_COROUTINE_POOL_THREADNUM;
std::unique_ptr<galay::coroutine::NetCoroutinePool> galay::coroutine::NetCoroutinePool::m_instance;
std::atomic_bool galay::coroutine::NetCoroutinePool::m_isStarted = false;

galay::coroutine::NetCoroutinePool::NetCoroutinePool()
{
    this->m_stop.store(true);
    this->m_isStopped.store(false);
    this->m_isDone.store(false);
}

void 
galay::coroutine::NetCoroutinePool::SetThreadNum(uint16_t threadNum)
{
    m_threadNum = threadNum;
}

galay::coroutine::NetCoroutinePool*
galay::coroutine::NetCoroutinePool::GetInstance()
{
    if(!m_instance)
    {
        m_instance = std::make_unique<NetCoroutinePool>();
        m_instance->Start();
    }
    return m_instance.get();
}

void 
galay::coroutine::NetCoroutinePool::Run()
{
    this->m_stop.store(false);
    while (!this->m_stop.load())
    {
        std::unique_lock lock(this->m_queueMtx);
        this->m_cond.wait_for(lock , std::chrono::milliseconds(5) , [this](){ return !this->m_waitCoQueue.empty() || this->m_stop.load(); });
        while(!this->m_eraseCoroutines.empty())
        {
            RealEraseCoroutine(*this->m_eraseCoroutines.begin());
            this->m_eraseCoroutines.erase(this->m_eraseCoroutines.begin());
        }
        std::vector<uint64_t> ids;
        while(!this->m_waitCoQueue.empty())
        {
            auto id = this->m_waitCoQueue.front();
            this->m_waitCoQueue.pop();
            ids.push_back(id);
        }
        lock.unlock();
        for(auto id: ids)
        {
            auto co = GetCoroutine(id);
            spdlog::debug("[{}:{}] [NetCoroutinePool.Run Resume CoId = {}]",__FILE__,__LINE__,id);
            co->Resume();
        }
        if(this->m_stop.load()) break;
    }
    spdlog::info("[{}:{}] [NetCoroutinePool Exit Normally]",__FILE__,__LINE__);
    if(--m_threadNum == 0) {
        m_exitCond.notify_one();
        this->m_isStopped.store(true);
    }
    
}

bool 
galay::coroutine::NetCoroutinePool::WaitForAllDone(uint32_t timeout)
{
    std::mutex mtx;
    std::unique_lock lock(mtx);
    if(timeout == 0){
        m_exitCond.wait(lock);
    }else{
        auto start = std::chrono::system_clock::now();
        m_exitCond.wait_for(lock, std::chrono::milliseconds(timeout));
        auto end = std::chrono::system_clock::now();
        if(std::chrono::duration_cast<std::chrono::milliseconds>(end - start) >= std::chrono::milliseconds(timeout)){
            if(m_isStopped.load() == true) return true;
            else return false;
        }
    }
    return true;
}

void 
galay::coroutine::NetCoroutinePool::Stop()
{
    if(!this->m_stop.load()){
        this->m_stop.store(true);
        spdlog::info("[{}:{}] [NetCoroutinePool.Stop]",__FILE__,__LINE__);
        this->m_cond.notify_all();
        this->m_isDone.store(true);
    }
}

bool 
galay::coroutine::NetCoroutinePool::IsDone()
{
    return m_isDone.load();
}

bool 
galay::coroutine::NetCoroutinePool::Contains(uint64_t coId)
{
    std::shared_lock lock(this->m_mapMtx);
    return this->m_coroutines.contains(coId);
}

bool 
galay::coroutine::NetCoroutinePool::Resume(uint64_t coId)
{
    if(!m_coroutines.contains(coId)) return false;
    this->m_waitCoQueue.push(coId);
    this->m_cond.notify_one();
    return true;
}

bool 
galay::coroutine::NetCoroutinePool::AddCoroutine(NetCoroutine::ptr coroutine)
{
    uint64_t coId = coroutine->GetCoId();
    std::unique_lock lock(this->m_mapMtx);
    if(coroutine->IsCoroutine()){
        {
            std::unique_lock lock(this->m_eraseMtx);
            if(this->m_eraseCoroutines.contains(coId)) this->m_eraseCoroutines.erase(coId);
        }
        this->m_coroutines.insert(std::make_pair(coId, coroutine));
        return true;
    }
    return false;
}

galay::coroutine::NetCoroutine::ptr 
galay::coroutine::NetCoroutinePool::GetCoroutine(uint64_t id)
{
    std::shared_lock lock(this->m_mapMtx);
    return m_coroutines[id];
}

void 
galay::coroutine::NetCoroutinePool::Start()
{
    if(!m_isStarted.load()){
        for(int i = 0 ; i < m_threadNum; i++)
        {
            auto th = std::make_unique<std::thread>([this](){
                Run();
            });
            th->detach();
            this->m_threads.push_back(std::move(th));
        }
        m_isStarted.store(true);
    }
}

bool 
galay::coroutine::NetCoroutinePool::EraseCoroutine(uint64_t coId)
{
    std::unique_lock lock(this->m_eraseMtx);
    if(!m_coroutines.contains(coId)) return false;
    this->m_eraseCoroutines.insert(coId);
    spdlog::debug("[{}:{}] [EraseCoroutine CoId={}]",__FILE__,__LINE__,coId);
    return true;
}

void 
galay::coroutine::NetCoroutinePool::RealEraseCoroutine(uint64_t coId)
{
    std::unique_lock lock(this->m_mapMtx);
    m_coroutines.erase(coId);
    spdlog::debug("[{}:{}] [RealEraseCoroutine CoId={}]",__FILE__,__LINE__,coId);
}

galay::coroutine::NetCoroutinePool::~NetCoroutinePool()
{
    if(!this->m_stop.load()){
        Stop();
    }
}



int 
galay::coroutine::TcpPromise::get_return_object_on_alloaction_failure() noexcept
{
    return -1;
}


galay::coroutine::NetCoroutine
galay::coroutine::TcpPromise::get_return_object() 
{ 
    SetStatus(CoroutineStatus::kCoroutineRunning);
    auto co = std::make_shared<galay::coroutine::NetCoroutine>(std::coroutine_handle<TcpPromise>::from_promise(*this));
    this->m_coId = co->GetCoId();
    NetCoroutinePool::GetInstance()->AddCoroutine(co);
    return std::move(*co); 
}


std::suspend_never
galay::coroutine::TcpPromise::initial_suspend() noexcept
{
    return {};
}


std::suspend_always
galay::coroutine::TcpPromise::yield_value(std::any value)
{
    SetResult(value);
    SetStatus(CoroutineStatus::kCoroutineSuspend);
    return {};
}


std::suspend_never
galay::coroutine::TcpPromise::final_suspend() noexcept
{
    SetStatus(CoroutineStatus::kCoroutineFinished);
    if(m_finishFunc != nullptr) m_finishFunc();
    NetCoroutinePool::GetInstance()->EraseCoroutine(m_coId);
    return {};
}


void galay::coroutine::TcpPromise::unhandled_exception() noexcept
{
    m_exception = std::current_exception();
}


void galay::coroutine::TcpPromise::return_value(std::any val) noexcept
{
    SetStatus(CoroutineStatus::kCoroutineFinished);
    SetResult(val);
}


std::any
galay::coroutine::TcpPromise::GetResult()
{
    rethrow_if_exception();
    return this->m_result;
}


void 
galay::coroutine::TcpPromise::SetResult(std::any result)
{
    this->m_result = result;
}


void 
galay::coroutine::TcpPromise::SetFinishFunc(std::function<void()> finshfunc)
{
    this->m_finishFunc = finshfunc;
}



galay::coroutine::CoroutineStatus 
galay::coroutine::TcpPromise::GetStatus()
{
    return this->m_status;
}



void 
galay::coroutine::TcpPromise::SetStatus(CoroutineStatus status)
{
    this->m_status = status;
}


void galay::coroutine::TcpPromise::rethrow_if_exception()
{
    if (m_exception)
    {
        std::rethrow_exception(m_exception);
    }
}

// int 
// galay::common::TcpPromise<void>::get_return_object_on_alloaction_failure()
// {
//     return -1;
// }

// std::coroutine_handle<galay::common::TcpPromise<void>>
// galay::common::TcpPromise<void>::get_return_object()
// {
//     SetStatus(CoroutineStatus::kCoroutineRunning);
//     return std::coroutine_handle<TcpPromise>::from_promise(*this);
// }

// std::suspend_never
// galay::common::TcpPromise<void>::initial_suspend() noexcept
// {
//     return {};
// }

// template <typename T>
// std::suspend_always 
// galay::common::TcpPromise<void>::yield_value(const T &value)
// {
//     SetStatus(CoroutineStatus::kCoroutineSuspend);
//     return {};
// }

// std::suspend_never
// galay::common::TcpPromise<void>::final_suspend() noexcept
// {
//     SetStatus(CoroutineStatus::kCoroutineFinished);
//     auto co = std::make_shared<NetCoroutine<void>>(std::coroutine_handle<TcpPromise>::from_promise(*this));
//     NetCoroutinePool::GetInstance()->AddCoroutine(co);
//     return {};
// }

// void 
// galay::common::TcpPromise<void>::unhandled_exception() noexcept
// {
//     m_exception = std::current_exception();
// }

// galay::coroutine::CoroutineStatus 
// galay::common::TcpPromise<void>::GetStatus()
// {
//     return this->m_status;
// }

// void 
// galay::common::TcpPromise<void>::SetStatus(CoroutineStatus status)
// {
//     this->m_status = status;
// }

// void 
// galay::common::TcpPromise<void>::SetFinishFunc(std::function<void()> finshfunc)
// {
//     this->m_finishFunc = finshfunc;
// }

// void galay::common::TcpPromise<void>::rethrow_if_exception()
// {
//     if (m_exception)
//     {
//         std::rethrow_exception(m_exception);
//     }
// }


//不能使用std::move 本质上m_handle是协程的引用，无法移交所有权，需要手动移动

galay::coroutine::Coroutine& 
galay::coroutine::Coroutine::operator=(Coroutine &&other)
{
    if(this != &other){
        this->m_handle = other.m_handle;
    }
    return *this;
}


galay::coroutine::Coroutine::Coroutine(std::coroutine_handle<promise_type> co_handle) noexcept
{
    this->m_handle = co_handle;
}


galay::coroutine::Coroutine::Coroutine(Coroutine &&other) noexcept
{
    if(this != &other) {
        this->m_handle = other.m_handle;
    }
}


void 
galay::coroutine::Coroutine::Resume() noexcept
{
    m_handle.resume();
}


bool 
galay::coroutine::Coroutine::Done() noexcept
{
    return m_handle.done();
}

uint64_t 
galay::coroutine::Coroutine::GetCoId() const noexcept
{
    if(m_handle == nullptr) return 0;
    return reinterpret_cast<uint64_t>(m_handle.address());
}


galay::coroutine::Coroutine::~Coroutine()
{
}



galay::coroutine::NetCoroutine::NetCoroutine(std::coroutine_handle<promise_type> co_handle) noexcept
    : Coroutine(co_handle)
{
}


galay::coroutine::NetCoroutine::NetCoroutine(NetCoroutine &&other) noexcept
    : Coroutine(std::forward<NetCoroutine &&>(other))
{
    
}


galay::coroutine::NetCoroutine& 
galay::coroutine::NetCoroutine::operator=(NetCoroutine &&other) noexcept
{
    Coroutine::operator=(std::forward<NetCoroutine &&>(other));
    return *this;
}

bool 
galay::coroutine::NetCoroutine::IsCoroutine()
{
    return this->m_handle != nullptr;
}



std::any
galay::coroutine::NetCoroutine::GetResult()
{
    if (!this->m_handle)
        return {};
    return this->m_handle.promise().GetResult();
}



galay::coroutine::CoroutineStatus 
galay::coroutine::NetCoroutine::GetStatus()
{
    if (!this->m_handle) return CoroutineStatus::kCoroutineNotExist;
    return this->m_handle.promise().GetStatus();
}



galay::coroutine::NetCoroutine::~NetCoroutine()
{
    
}
