#include "coroutine.h"
#include "signalhandler.h"
#include <csignal>

galay::common::GY_NetCoroutinePool::GY_NetCoroutinePool(std::shared_ptr<std::condition_variable> fcond)
{
    this->m_stop.store(false);
    this->m_fcond = fcond;
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
            this->m_eraseCoroutines.pop();
        }
        while(!this->m_waitCoQueue.empty())
        {
            auto id_always = this->m_waitCoQueue.front();
            this->m_waitCoQueue.pop();
            if(Contains(id_always.first)){
                auto& co = GetCoroutine(id_always);
                spdlog::info("[{}:{}] [GY_NetCoroutinePool.Start CoId = {}, WaiQueue Size = {}]",__FILE__,__LINE__,co.GetCoId(),m_waitCoQueue.size());
                if(co.IsCoroutine() && !co.Done() && co.GetStatus() != kCoroutineFinished) co.Resume();
                else RealEraseCoroutine(id_always.first);
            }
        }
        if(this->m_stop.load()) break;
    }
    spdlog::info("[{}:{}] [GY_NetCoroutinePool Exit Normally]",__FILE__,__LINE__);
    this->m_fcond->notify_all();
}

void 
galay::common::GY_NetCoroutinePool::Stop()
{
    if(!this->m_stop.load()){
        this->m_stop.store(true);
        spdlog::info("[{}:{}] [GY_NetCoroutinePool.Stop]",__FILE__,__LINE__);
        this->m_cond.notify_all();
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
        spdlog::info("[{}:{}] [GY_NetCoroutinePool.AddCoroutine CoId = {}]",__FILE__,__LINE__,coroutine.GetCoId());
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