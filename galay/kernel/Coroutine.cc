#include "Coroutine.hpp"
#include "Time.h"

namespace galay
{
RoutineSharedCtx::ptr RoutineSharedCtx::Create()
{
    return std::make_shared<RoutineSharedCtx>(CoroutineSchedulerHolder::GetInstance()->GetScheduler());
}

RoutineSharedCtx::RoutineSharedCtx(details::CoroutineScheduler *scheduler)
    : m_scheduler(scheduler)
{
}

RoutineSharedCtx::RoutineSharedCtx(const RoutineSharedCtx &ctx)
{
    this->m_scheduler.store(ctx.m_scheduler);
}

RoutineSharedCtx::RoutineSharedCtx(RoutineSharedCtx &&ctx)
{
    this->m_scheduler.store(std::move(ctx.m_scheduler));
    ctx.m_scheduler.store(nullptr);
}

details::CoroutineScheduler *RoutineSharedCtx::GetScheduler()
{
    return m_scheduler.load();
}

int32_t RoutineSharedCtx::AddToGraph(uint16_t layer, std::weak_ptr<CoroutineBase> coroutine)
{
    if(layer >= m_coGraph.size()) {
        m_coGraph.resize(layer + 1);
    }
    int32_t size = m_coGraph[layer].size();
    m_coGraph[layer].emplace(size, coroutine);
    return size;
}

void RoutineSharedCtx::RemoveFromGraph(uint16_t layer, int32_t sequence)
{
    if(layer >= m_coGraph.size()) {
        return;
    }
    m_coGraph[layer].erase(sequence);
}

RoutinePrivateCtx::ptr RoutinePrivateCtx::Create()
{
    return std::make_shared<RoutinePrivateCtx>();
}

RoutinePrivateCtx::RoutinePrivateCtx()
{
}

uint16_t& RoutinePrivateCtx::GetThisLayer()
{
    return m_layer;
}

int32_t& RoutinePrivateCtx::GetThisSequence()
{
    return m_sequence;
}

std::vector<std::unordered_map<int32_t, std::weak_ptr<CoroutineBase>>> &RoutineSharedCtx::GetRoutineGraph()
{
    return m_coGraph;
}

RoutineCtx RoutineCtx::Create()
{
    return RoutineCtx(RoutineSharedCtx::Create());
}

RoutineCtx RoutineCtx::Create(details::CoroutineScheduler *scheduler)
{
    return RoutineCtx(std::make_shared<RoutineSharedCtx>(scheduler));
}

RoutineCtx::RoutineCtx(RoutineSharedCtx::ptr sharedData)
    : m_sharedCtx(sharedData), m_privateCtx(RoutinePrivateCtx::Create())
{
}

RoutineCtx::RoutineCtx(const RoutineCtx &ctx)
{
    this->m_sharedCtx = ctx.m_sharedCtx;
    this->m_privateCtx = ctx.m_privateCtx;
}

RoutineCtx::RoutineCtx(RoutineCtx &&ctx)
{
    this->m_sharedCtx = ctx.m_sharedCtx;
    ctx.m_sharedCtx.reset();
    this->m_privateCtx = ctx.m_privateCtx;
    ctx.m_privateCtx.reset();
}

RoutineCtx RoutineCtx::Copy()
{
    RoutineCtx ctx(m_sharedCtx);
    ctx.m_privateCtx->m_layer = m_privateCtx->m_layer;
    ctx.m_privateCtx->m_sequence = m_privateCtx->m_sequence;
    return std::move(ctx);
}

RoutineSharedCtx::wptr RoutineCtx::GetSharedCtx()
{
    return m_sharedCtx;
}

uint16_t RoutineCtx::GetThisLayer() const
{
    return m_privateCtx->m_layer;
}

}