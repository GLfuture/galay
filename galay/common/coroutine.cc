#include "coroutine.h"
galay::common::GY_NetCoroutinePool::GY_NetCoroutinePool()
{
    this->m_stop.store(false);
    this->m_isStopped.store(false);
}

void 
galay::common::GY_NetCoroutinePool::Start()
{
    while (!this->m_stop.load())
    {
        std::unique_lock lock(this->m_queueMtx);
        this->m_cond.wait_for(lock , std::chrono::milliseconds(5) , [this](){ return !this->m_waitCoQueue.empty() || this->m_stop.load(); });
        while(!this->m_eraseCoroutines.empty())
        {
            RealEraseCoroutine(this->m_eraseCoroutines.front());
            spdlog::debug("[{}:{}] [GY_NetCoroutinePool Erase CoId = {}]",__FILE__,__LINE__,this->m_eraseCoroutines.front());
            this->m_eraseCoroutines.pop();
        }
        while(!this->m_waitCoQueue.empty())
        {
            auto id_always = this->m_waitCoQueue.front();
            this->m_waitCoQueue.pop();
            spdlog::debug("[{}:{}] [GY_NetCoroutinePool Resume CoId = {}]",__FILE__,__LINE__,id_always.first);
            if(Contains(id_always.first)){
                auto& co = GetCoroutine(id_always);
                if(co.IsCoroutine() && !co.Done() && co.GetStatus() != kCoroutineFinished) co.Resume();
                else RealEraseCoroutine(id_always.first);
            }
        }
        if(this->m_stop.load()) break;
    }
    spdlog::info("[{}:{}] [GY_NetCoroutinePool Exit Normally]",__FILE__,__LINE__);
    m_exitCond.notify_one();
    this->m_isStopped.store(true);
}

bool 
galay::common::GY_NetCoroutinePool::WaitForAllDone(uint32_t timeout)
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
galay::common::GY_NetCoroutinePool::Stop()
{
    if(!this->m_stop.load()){
        this->m_stop.store(true);
        spdlog::info("[{}:{}] [GY_NetCoroutinePool.Stop]",__FILE__,__LINE__);
        this->m_cond.notify_one();
    }
}

bool 
galay::common::GY_NetCoroutinePool::Contains(uint64_t coId)
{
    std::shared_lock lock(this->m_mapMtx);
    return this->m_coroutines.contains(coId);
}

bool 
galay::common::GY_NetCoroutinePool::Resume(uint64_t coId, bool always)
{
    if(!m_coroutines.contains(coId)) return false;
    this->m_waitCoQueue.push(std::make_pair(coId, always));
    spdlog::info("[{}:{}] [GY_NetCoroutinePool.Resume CoId = {}]", __FILE__ , __LINE__ , coId);
    this->m_cond.notify_one();
    return true;
}

bool 
galay::common::GY_NetCoroutinePool::AddCoroutine(uint64_t coId, GY_NetCoroutine<CoroutineStatus>&& coroutine)
{
    std::unique_lock lock(this->m_mapMtx);
    if(coroutine.IsCoroutine()){
        spdlog::info("[{}:{}] [GY_NetCoroutinePool.AddCoroutine CoId = {}]",__FILE__,__LINE__,coId);
        this->m_coroutines.insert(std::make_pair(coId, std::forward<GY_NetCoroutine<CoroutineStatus>>(coroutine)));
        return true;
    }
    return false;
}

galay::common::GY_NetCoroutine<galay::common::CoroutineStatus>& 
galay::common::GY_NetCoroutinePool::GetCoroutine(std::pair<uint64_t,bool> id_always)
{
    std::shared_lock lock(this->m_mapMtx);
    return m_coroutines[id_always.first];
}

bool 
galay::common::GY_NetCoroutinePool::EraseCoroutine(uint64_t coId)
{
    if(!m_coroutines.contains(coId)) return false;
    this->m_eraseCoroutines.push(coId);
    spdlog::info("[{}:{}] [EraseCoroutine CoId={}]",__FILE__,__LINE__,coId);
    return true;
}

void 
galay::common::GY_NetCoroutinePool::RealEraseCoroutine(uint64_t coId)
{
    std::unique_lock lock(this->m_mapMtx);
    m_coroutines.erase(coId);
    spdlog::info("[{}:{}] [RealEraseCoroutine CoId={}]",__FILE__,__LINE__,coId);
}

galay::common::GY_NetCoroutinePool::~GY_NetCoroutinePool()
{
    if(!this->m_stop.load()){
        Stop();
    }
}