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
    : m_sharedCtx(sharedData), m_layer(0), m_sequence(-1)
{
}

RoutineCtx::RoutineCtx(const RoutineCtx &ctx)
{
    this->m_sharedCtx = ctx.m_sharedCtx;
    this->m_layer = ctx.m_layer + 1;
    this->m_sequence = -1;
}

RoutineCtx::RoutineCtx(RoutineCtx &&ctx)
{
    this->m_sharedCtx = ctx.m_sharedCtx;
    ctx.m_sharedCtx.reset();
    this->m_layer = ctx.m_layer;
    this->m_sequence = ctx.m_sequence;
}

RoutineSharedCtx::wptr RoutineCtx::GetSharedCtx()
{
    return m_sharedCtx;
}

uint16_t RoutineCtx::GetThisLayer() const
{
    return m_layer;
}
}